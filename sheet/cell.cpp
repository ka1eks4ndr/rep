#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

bool Cell::IsReferenced () const {
    return !dependent_cells_.empty();
}

bool Cell::CellCircularReferences (Position cur_pos, std::set<Position>referenced_cells) {
    bool circular_references = false;
    for (const auto& pos : referenced_cells) {
        if (pos == cur_pos) {
            return true;
        }
    }
    for (const auto& pos : referenced_cells) {
        auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        circular_references = CellCircularReferences(cur_pos,cell->referenced_cells_ );
        if (circular_references) {
            return circular_references;
        }
    }
    return circular_references;
}

Cell::Cell (Sheet& sheet) :  
                    sheet_(sheet), 
                    impl_(std::make_unique<EmptyImpl>()) {
}

Cell::~Cell() {}

void Cell::Set(std::string text, Position cur_pos)  {
    if (text.empty()) {
        ClearAllReference(cur_pos, referenced_cells_);
        impl_ = std::make_unique<EmptyImpl>();
        ClearDependentCache(dependent_cells_);
    } else if (text[0] == '=' && text.size() > 1)  {
        try {
            auto formula = ParseFormula(text.substr(1, text.size() - 1));
            auto ref_cels = formula->GetReferencedCells();
            std::set<Position>referenced_cells_temp = {ref_cels.begin(),ref_cels.end()};
            if (!ref_cels.empty()) {
                
                for (const auto& pos : referenced_cells_temp) {
                    if (!sheet_.GetCell(pos)) {
                        sheet_.SetCell(pos,"");
                    }
                }
                if (CellCircularReferences(cur_pos, referenced_cells_temp)) {
                    throw CircularDependencyException("this formula \"" + text+"\" have a CircularDependency ");
                }
            }
            ClearAllReference(cur_pos, referenced_cells_);
            referenced_cells_ = referenced_cells_temp;
            impl_ = std::make_unique<FormulaImpl>(std::move(formula));
            ClearDependentCache(dependent_cells_);
            SetDependentCell(cur_pos, referenced_cells_);
        } catch (const FormulaException&) {
            throw;
        } 
    } else {
        impl_ = std::make_unique<TextImpl>(text);
        ClearAllReference(cur_pos,referenced_cells_);
        ClearDependentCache(dependent_cells_);
    }
}

void Cell::Clear(Position cur_pos) {
    ClearAllReference(cur_pos, referenced_cells_);
    impl_ = std::make_unique<EmptyImpl>();
    ClearDependentCache(dependent_cells_);
}

Cell::Value Cell::GetValue() const  {
    return impl_->GetValue(this);
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return {referenced_cells_.begin(),referenced_cells_.end()};
}

void Cell::ClearDependentCell(Position cur_pos) {
    dependent_cells_.erase(cur_pos);
}

void Cell::ClearAllReference(Position cur_pos, std::set<Position>referenced_cells) {
    for (const auto& pos : referenced_cells) {
       auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
       cell->ClearDependentCell(cur_pos);
       ClearAllReference(cur_pos,cell->referenced_cells_);
    }
    referenced_cells_.clear();
}

void Cell::ClearDependentCache(std::set<Position>dependent_cells) {
    for (const auto& pos : dependent_cells) {
        auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        cell->ClearCache();
        ClearDependentCache(cell->dependent_cells_);
    }
}

void Cell::ClearCache() {
    impl_->ClearCache();
}

void Cell::SetDependentCell(Position cur_pos, std::set<Position>referenced_cells) {
    for (const auto& pos : referenced_cells) {
        auto cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        cell->dependent_cells_.insert(cur_pos);
        SetDependentCell(cur_pos, cell->referenced_cells_);
    }
}

//-----------EmptyImpl----------------


Cell::Value Cell::EmptyImpl::GetValue(const Cell* /*cell*/) const {
    return "";
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}

//-----------TextImpl----------------
     
Cell::TextImpl::TextImpl (std::string str) :
    // Impl(str) 
    str_(str){
}

Cell::Value Cell::TextImpl::GetValue(const Cell* /*cell*/) const {
    
    if (str_[0] == '\'') {
        return str_.substr(1, str_.size() -1);
    } else {
        return str_;
    }
}

std::string Cell::TextImpl::GetText() const {
    return str_;
}

//-----------FormulaImpl----------------

     
Cell::FormulaImpl::FormulaImpl (std::unique_ptr<FormulaInterface>&& formula) :
    formula_(std::move(formula)) {
}

Cell::Value Cell::FormulaImpl::GetValue(const Cell* cell ) const {
    if (value_) {
        return *value_;
    }
    const auto& sheet = cell->sheet_;
    if (std::holds_alternative<FormulaError>(formula_->Evaluate(sheet))) {
        return std::get<FormulaError>(formula_->Evaluate(sheet));
    } else {
        value_ = std::get<double>(formula_->Evaluate(sheet));
        return *value_;
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return "=" + formula_->GetExpression();
}

void Cell::FormulaImpl::ClearCache() {
    value_ = std::nullopt;
}