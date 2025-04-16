#include "displayMessage.h"
#include <string.h>
#include <string>

DisplayMessage::DisplayMessage()
{
    memset(content,'\0',sizeof(content));
};

DisplayMessage::DisplayMessage(int _targetSegment, std::string &message) : DisplayMessage()
{
    targetSegment = _targetSegment;
    strlcpy(content,message.c_str(),sizeof(content));
};
