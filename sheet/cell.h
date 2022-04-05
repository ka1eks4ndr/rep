#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <set>

class Sheet;

class Cell : public CellInterface {
public:
        Cell(Sheet& sheet);
        ~Cell();

        void Set(std::string text, Position cur_pos);
        void Clear(Position cur_pos);

        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

        bool IsReferenced() const;
        void ClearCache();

private:
        class Impl {
        public:
            virtual Value GetValue(const Cell* cell) const = 0;
            virtual std::string GetText() const = 0;
            virtual void ClearCache() {}
        
        };

        class EmptyImpl : public Impl {
        public:
            Value GetValue(const Cell* /*cell*/) const override;
            std::string GetText() const override;
            
        };

        class TextImpl : public Impl {
        public:
            TextImpl (std::string str);
            Value GetValue(const Cell* /*cell*/) const override;
            std::string GetText() const override;
        private:
            std::string str_;
        };

        class FormulaImpl : public Impl {
        public:
            FormulaImpl (std::unique_ptr<FormulaInterface>&& formula);
            Value GetValue(const Cell* cell ) const override;
            std::string GetText() const override;
            void ClearCache();
        private:
            std::unique_ptr<FormulaInterface> formula_ = nullptr;
            mutable std::optional<double> value_;
        };

         
        bool CellCircularReferences (Position cur_pos, std::set<Position>referenced_cells);
        void ClearDependentCache(std::set<Position>dependent_cells);
        void ClearAllReference(Position cur_pos, std::set<Position>referenced_cells);
        void SetDependentCell(Position cur_pos, std::set<Position>referenced_cells);
        void ClearDependentCell(Position cur_pos);

        Sheet& sheet_;
        std::unique_ptr<Impl> impl_;
        std::set<Position>referenced_cells_;
        std::set<Position>dependent_cells_;
};