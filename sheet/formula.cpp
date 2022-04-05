#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) : 
             ast_(ParseFormulaAST(expression)) {
    }

    Value Evaluate(const SheetInterface& sheet) const override { 
        double value;
        std::function<const CellInterface*(Position pos)> get_cell = 
            [&](Position pos){return sheet.GetCell(pos);};
        try {
            value = ast_.Execute(get_cell);
            return value;
        } catch (const FormulaError& e) {
            return e;
        } 
    }

    std::string GetExpression() const override {
        std::stringstream so;
       ast_.PrintFormula(so);
       return so.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        return ast_.GetReferencedCells();
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (std::exception& e) {
        throw FormulaException(e.what());
    }
    catch (...) {
        throw FormulaException("Unknown exception caught");
    }
}