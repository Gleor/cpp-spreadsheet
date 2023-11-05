#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <unordered_map>

class PositionHasher {
public:
	size_t operator()(const Position pos) const {
		return std::hash<std::string>()(pos.ToString());
	}
};

class Sheet : public SheetInterface {
public:
	using SheetData = std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher>;

	~Sheet();

	void SetCell(Position position, std::string text) override;

	CellInterface* GetCell(Position position) override;
	const CellInterface* GetCell(Position position) const override;

	Cell* GetCellPtr(Position position);
	const Cell* GetCellPtr(Position position) const;

	void ClearCell(Position position) override;

	Size GetPrintableSize() const override;

	void PrintValues(std::ostream& output) const override;
	void PrintTexts(std::ostream& output) const override;

private:
	SheetData data_;
};