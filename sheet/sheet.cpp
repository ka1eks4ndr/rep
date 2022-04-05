#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

struct ValuePrinter {
    std::ostream& out;
    void operator()(std::string value) const {
        out << value;
    }
    void operator()(double value) const {
        out << value;
    }
    void operator()(FormulaError value) const {
        out << value;
    }
};

Sheet::Sheet() {}  
   
Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("out of range position"); 
    }
    
    auto size = GetPrintableSize();
    if (size.cols <= pos.col) {
        for (auto& element : sheet_ ) {
            element.resize(pos.col + 1);
        }
        printable_size_.cols = pos.col + 1;
        size = GetPrintableSize();
    }
    if (size.rows <= pos.row) {
        int init_col = 0;
        if (sheet_.empty()) {
            init_col = pos.col + 1;
        } else {
            init_col = size.cols;
        }
        for (int i = 0; i <= pos.row - size.rows; ++i ) {
            sheet_.push_back(std::vector<std::unique_ptr<Cell>>(init_col));
        }
        printable_size_.rows = pos.row + 1;
    }
    if (sheet_[pos.row][pos.col]) {
        sheet_[pos.row][pos.col]->Set(text, pos); 
    } else {
        sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this);
        sheet_[pos.row][pos.col]->Set(text, pos);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
   if (!pos.IsValid()) {
        throw InvalidPositionException("out of range position"); 
    }
    auto size = GetPrintableSize();
    if (size.cols <= pos.col || size.rows <= pos.row ) {
        return nullptr;
    } else {
        if (!(sheet_[pos.row][pos.col])) {
            return nullptr;
        } else {
                return &(*(sheet_[pos.row][pos.col]));
        }
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("out of range position"); 
    }
    auto size = GetPrintableSize();
    if (size.cols <= pos.col || size.rows <= pos.row ) {
        return nullptr;
    } else {
        if (!(sheet_[pos.row][pos.col])) {
            return nullptr;
        } else {
                return &(*(sheet_[pos.row][pos.col]));
        }
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("out of range position"); 
    }
    auto size = GetPrintableSize();
    if (size.cols <= pos.col || size.rows <= pos.row ) {
        return;
    }
    if (sheet_[pos.row][pos.col]) {
        sheet_[pos.row][pos.col]->Clear(pos);
        DelEmptyRows();
        DelEmptyCols();
    }
}

void Sheet::DelEmptyRows () {
    int index = printable_size_.rows - 1; 
    while (index >= 0 && IsEmptyRow(index)) {
        --printable_size_.rows;
        --index;
    }
}

bool Sheet::IsEmptyRow (int index) {
    auto result = std::find_if_not(sheet_[index].begin(), sheet_[index].end(), [](const auto& cell){
                                                if (cell) {
                                                    return cell->GetText() == "";
                                                } else {
                                                    return true;
                                                }
    });
    if (result == sheet_[index].end()) {
        return true;
    } 
    return false; 
}

void Sheet::DelEmptyCols () {
    if (printable_size_ == Size{0,0}) {
        return;
    }
    int index = printable_size_.cols - 1;
    while (index >= 0 && IsEmptyCol(index)) {
        --printable_size_.cols;
        --index;
    }
}

bool Sheet::IsEmptyCol (int index) {
    auto result = std::find_if_not(sheet_.begin(), sheet_.end(), [index](const std::vector<std::unique_ptr<Cell>>& row){
                                                        if (row.at(index)) {
                                                            return row.at(index)->GetText() == "";
                                                        } else {
                                                            return true;
                                                        }
    });
    if (result == sheet_.end()) {
        return true;
    }
    return false;
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        bool first = true;
        for (int j = 0; j < printable_size_.cols; ++j) {
            if (first) {
                if (sheet_[i][j]) {
                    std::visit(ValuePrinter{output}, sheet_[i][j]->GetValue());
                }
                first = false;
                continue;
            }
            output << '\t';
            if (sheet_[i][j]) {
                    std::visit(ValuePrinter{output}, sheet_[i][j]->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const { 
    for (int i = 0; i < printable_size_.rows; ++i) {
        bool first = true;
        for (int j = 0; j < printable_size_.cols; ++j) {
            if (first) {
                if (sheet_[i][j]) {
                    output << sheet_[i][j]->GetText();
                }
                first = false;
                continue;
            }
            output << '\t';
            if (sheet_[i][j]) {
                output << sheet_[i][j]->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}