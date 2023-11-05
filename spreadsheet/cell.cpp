#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <set>
#include <stack>
#include <string>

#include "sheet.h"

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string content, Position position) {

    if (content == impl_->GetText()) {
        return;
    }

    std::unique_ptr<Impl> impl;

    if (content.empty()) {
        impl = std::make_unique<EmptyImpl>();
    }

    if (content.size() >= 2 && content[0] == FORMULA_SIGN) {
        impl = std::make_unique<FormulaImpl>(std::move(content), sheet_);

        for (Position cell : impl->GetReferencedCells()) {
            if (cell.IsValid() && !sheet_.GetCell(cell)) {
                sheet_.SetCell(cell, "");
            }
        }
    }
    else {
        impl = std::make_unique<TextImpl>(std::move(content));
    }

    if (CheckCircularDependency(*impl, position)) {
        throw CircularDependencyException("Circular dependency");
    }

    impl_ = std::move(impl);

    for (Cell* cell : ancestors_) {
        cell->descendants_.erase(this);
    }

    ancestors_.clear();

    for (const auto& referenced_cell : impl_->GetReferencedCells()) {
        Cell* cell = sheet_.GetCellPtr(referenced_cell);

        if (!cell) {
            sheet_.SetCell(referenced_cell, "");
            cell = sheet_.GetCellPtr(referenced_cell);
        }

        ancestors_.insert(cell);
        cell->descendants_.insert(this);
    }

    ClearCache();
}

bool Cell::InternalCircularDependencyChecker(Cell* cell, std::unordered_set<Cell*>& cells,
    const Position position) {
    for (const Position& cell : cell->GetReferencedCells()) {
        if (position == cell) {
            return true;
        }
        Cell* referenced_cell = sheet_.GetCellPtr(cell);
        if (cells.find(referenced_cell) == cells.end()) {
            cells.insert(referenced_cell);
            if (InternalCircularDependencyChecker(referenced_cell, cells, position)) return true;
        }
    }

    return false;
}

bool Cell::CheckCircularDependency(const Impl& impl, Position position) {

    const Position position_const = position;
    std::unordered_set<Cell*> cells;

    for (const Position& cell : impl.GetReferencedCells()) {
        if (cell == position) {
            return true;
        }
        Cell* referenced_cell = sheet_.GetCellPtr(cell);
        cells.insert(referenced_cell);
        if (InternalCircularDependencyChecker(referenced_cell, cells, position_const)) return true;
    }

    return false;
}

void Cell::Clear() { Set("", position_); }

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }

std::string Cell::GetText() const { return impl_->GetText(); }

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const { return !descendants_.empty(); }

void Cell::ClearCache() {
    if (impl_->IsCacheValid()) {
        impl_->ClearCache();

        for (Cell* cell : descendants_) {
            cell->ClearCache();
        }
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const { return {}; }

bool Cell::Impl::IsCacheValid() { return false; }
void Cell::Impl::ClearCache() {}

Cell::Value Cell::EmptyImpl::GetValue() const { return ""; }
std::string Cell::EmptyImpl::GetText() const { return ""; }

Cell::TextImpl::TextImpl(std::string content) : text_(std::move(content)) {}

Cell::Value Cell::TextImpl::GetValue() const {
    if (text_.empty()) {
        throw FormulaException("Empty cell");
    }

    return text_.at(0) == ESCAPE_SIGN ? text_.substr(1) : text_;
}

std::string Cell::TextImpl::GetText() const { return text_; }

Cell::FormulaImpl::FormulaImpl(std::string content, SheetInterface& sheet)
    : sheet_(sheet)
    , formula_(ParseFormula(content.substr(1))) {}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_) {
        cache_ = formula_->Evaluate(sheet_);
    }
    return std::visit([](auto& helper) { return Value(helper); }, *cache_);
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

bool Cell::FormulaImpl::IsCacheValid() { return cache_.has_value(); }
void Cell::FormulaImpl::ClearCache() { cache_.reset(); }
