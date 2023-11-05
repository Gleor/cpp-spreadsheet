#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <set>
#include <stack>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string content, Position pos);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
    void ClearCache();

private:
    class Impl;

    bool InternalCircularDependencyChecker(Cell* cell, std::unordered_set<Cell*>& cells,
        const Position position);
    bool CheckCircularDependency(const Impl& impl, Position position);

    class Impl {
    public:
        virtual ~Impl() = default;

        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;

        virtual bool IsCacheValid();
        virtual void ClearCache();
    };

    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string content);
        Value GetValue() const override;
        std::string GetText() const override;

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string content, SheetInterface& sheet);

        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

        bool IsCacheValid() override;
        void ClearCache() override;

    private:
        SheetInterface& sheet_;
        std::unique_ptr<FormulaInterface> formula_;
        mutable std::optional<FormulaInterface::Value> cache_;
    };

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    
    std::unordered_set<Cell*> ancestors_;
    std::unordered_set<Cell*> descendants_;

    Position position_;
};