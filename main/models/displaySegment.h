#pragma once

#include "./scrollData.h"
#include "./displayMessage.h"
#include "bounds.h"
#include "string.h"
#include "esp_log.h"
#include <string>

struct DisplaySegment
{
public:
    DisplaySegment() {};
    DisplaySegment(int segmentId, int layerNumber, int startColumn, int startRow, int endColumn, int endRow);
    DisplaySegment(int segmentId, int layerNumber, Bounds bounds);
    void SetContent(std::string &content);
    std::string GetContent();
    int id;
    int layerNumber;
    ScrollData scrollData;
    Bounds bounds;

private:
    std::string _content;
};
