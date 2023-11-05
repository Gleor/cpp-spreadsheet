#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {

    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position");
    }

    auto [cell_ref, success] = data_.emplace(pos, std::make_unique<Cell>(*this));

    cell_ref->second->Set(std::move(text), std::move(pos));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellPtr(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPtr(pos);
}
const Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");

    const auto iter = data_.find(pos);
    if (iter == data_.end()) {
        return nullptr;
    }

    return iter->second.get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellPtr(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position");
    }

    const auto& cell = data_.find(pos);
    if (cell != data_.end() && cell->second != nullptr) {
        cell->second->Clear();
        if (!cell->second->IsReferenced()) {
            cell->second.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size size;

    for (auto it = data_.begin(); it != data_.end(); ++it) {
        if (it->second != nullptr) {
            const int col = it->first.col;
            const int row = it->first.row;
            size.rows = std::max(size.rows, row + 1);
            size.cols = std::max(size.cols, col + 1);
        }
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {

    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) output << '\t';
            const auto& iter = data_.find({ row, col });
            if (iter != data_.end() && iter->second != nullptr && !iter->second->GetText().empty()) {
                std::visit([&](const auto value) { output << value; }, iter->second->GetValue());
            }
        }

        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {

    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col) output << '\t';
            const auto& iter = data_.find({ row, col });
            if (iter != data_.end() && iter->second != nullptr && !iter->second->GetText().empty()) {
                output << iter->second->GetText();
            }
        }

        output << '\n';
    }
}
std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}