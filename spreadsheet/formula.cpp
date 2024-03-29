#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <regex>
#include <sstream>

#include "FormulaAST.h"

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError formula_error) {
    return output << formula_error.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression) try
            : formula_ast_(ParseFormulaAST(expression)) {
        }
        catch (...) {
            throw FormulaException("Failed to parse formula"s);
        }

        Value Evaluate(const SheetInterface& sheet) const {
            try {
                std::function<double(Position)> func =
                    [&sheet](const Position position) -> double {
                    if (!position.IsValid()) {
                        throw FormulaError(FormulaError::Category::Ref);
                    }

                    const auto* cell = sheet.GetCell(position);
                    if (!cell) {
                        return 0.0;
                    }

                    const auto& value = cell->GetValue();
                    if (std::holds_alternative<double>(value)) {
                        return std::get<double>(value);
                    }

                    if (std::holds_alternative<std::string>(value)) {
                        const auto& str = std::get<std::string>(value);
                        double result = 0;
                        if (!str.empty()) {
                            std::istringstream in(str);
                            if (!(in >> result) || !in.eof()) {
                                throw FormulaError(FormulaError::Category::Value);
                            }
                        }
                        return result;
                    }

                    throw FormulaError(std::get<FormulaError>(value));
                    };

                return formula_ast_.Execute(func);
            }
            catch (const FormulaError& formula_error) {
                return formula_error;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream out;
            formula_ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            std::vector<Position> cell_positions;
            for (const auto& cell : formula_ast_.GetCells()) {
                if (cell.IsValid()) {
                    cell_positions.push_back(cell);
                }
            }
            std::sort(cell_positions.begin(), cell_positions.end());
            cell_positions.erase(
                std::unique(cell_positions.begin(), cell_positions.end()),
                cell_positions.end());
            return cell_positions;
        }

    private:
        FormulaAST formula_ast_;
    };

}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}