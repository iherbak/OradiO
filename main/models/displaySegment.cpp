#include "displaySegment.h"

DisplaySegment::DisplaySegment(int segmentId, int layer, int startColumn, int startRow, int endColumn, int endRow)
{
    id = segmentId;
    layerNumber = layer;
    bounds.startRow = startRow;
    bounds.startColumn = startColumn;
    bounds.endColumn = endColumn;
    bounds.endRow = endRow;
    _content = "";
    scrollData.isScrolling = 0;
    scrollData.scrollPosition = -1;
};

DisplaySegment::DisplaySegment(int segmentId, int layer, Bounds bounds) : DisplaySegment(segmentId, layer, bounds.startColumn, bounds.startRow, bounds.endColumn, bounds.endRow) {
};

std::string DisplaySegment::GetContent()
{
    return _content;
};

void DisplaySegment::SetContent(std::string &content)
{
    if (_content != content)
    {
        _content = content;
        // copy over content and extend it if shorter
        int extraSpace = (bounds.endColumn - bounds.startColumn) - _content.length();
        ESP_LOGD("Segment", "extraspace %d", extraSpace);
        if (extraSpace > 0)
        {
            scrollData.isScrolling = false;
            for (int i = 0; i < extraSpace; i++)
            {
                _content += " ";
            }
            ESP_LOGD("Segment", "Padded is %s with length %d", _content.c_str(), _content.length());
        }
        else
        {
            scrollData.isScrolling = true;
            scrollData.scrollPosition = -1;
        }
    }
};
