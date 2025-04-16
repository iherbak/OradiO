#pragma once
#include "scrollData.h"
#include <string>

class DisplayMessage
{
public:
    DisplayMessage(int targetSegment, std::string &message);
    int targetSegment;
    char content[MAX_DISPLAY_MESSAGE_LENGTH];
private:
    DisplayMessage();
};
