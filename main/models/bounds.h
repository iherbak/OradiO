#pragma once

class Bounds
{
public:
    Bounds() : Bounds(0,0,0,0) {};
    Bounds(int _startRow,int _startColumn, int _endColumn, int _endRow) {
        startRow = _startRow;
        startColumn = _startColumn;
        endColumn = _endColumn;
        endRow = _endRow;
    };
    int startRow;
    int startColumn;
    int endColumn;
    int endRow;
};
