#pragma once

#include <string>
#include <string.h>
#define BUFLEN 170

// #define BIG_A 1      // Á
#define SMALL_A 1 // á
// #define BIG_E 3      // É
#define SMALL_E 2 // é
// #define BIG_I 5      // Í
#define SMALL_I 3 // í
// #define BIG_O 7      // Ó
#define SMALL_O 4 // ó
// #define BIG_OO 9     // Ö
#define SMALL_OO 5 // ö
// #define BIG_OOO 11   // Ő
// #define SMALL_OOO 5 // ő
// #define BIG_U 13     // Ú
#define SMALL_U 6 // ú
// #define BIG_UU 15    // Ü
#define SMALL_UU 7 // ü
// #define BIG_UUU 17   // Ű
// #define SMALL_UUU 8 // ű

static char *convertUsingCustomChars(const char *str, bool uppercase)
{

  int index = 0;
  static char strn[BUFLEN];
  static char newStr[BUFLEN];
  strlcpy(strn, str, BUFLEN);

  uint8_t charIndex;
  uint8_t utf2ndByte;
  uint8_t utf3rdByte;
  uint8_t utf4thdByte;
  int addToStr = 0;

  while (*str)
  {
    charIndex = tolower(*str);

    uint16_t code = *str;
    addToStr = 1;
    // two bytes UTF8
    if (charIndex > 192)
    {
      utf2ndByte = *(str + 1);
      if (utf2ndByte >= 128)
      {
        code = ((*str & 0x1F) << 6) | (*(str + 1) & 0x3F);
        addToStr += 1;
      }
    }
    // three bytes
    if (charIndex > 224)
    {
      utf2ndByte = *(str + 1);
      if (utf2ndByte >= 128)
      {
        utf3rdByte = *(str + 2);
        if (utf3rdByte >= 128)
        {
          code = ((*str & 0x0F) << 12) | ((*(str + 1) & 0x3F) << 6) | (*(str + 2) & 0x3F);
          addToStr += 2;
        }
      }
    }
    // four bytes
    if (charIndex > 240)
    {
      utf2ndByte = *(str + 1);
      if (utf2ndByte >= 128)
      {
        utf3rdByte = *(str + 2);
        if (utf3rdByte >= 128)
        {
          utf4thdByte = *(str + 3);
          if (utf4thdByte >= 128)
          {
            code = ((*str & 0x07) << 18) | ((*(str + 1) & 0x3F) << 12) | ((*(str + 2) & 0x3F) << 6) | (*(str + 3) & 0x3F);
            addToStr += 3;
          }
        }
      }
    }

    switch (code)
    {
    case 0x00C1:
    case 0x00E1:
      charIndex = SMALL_A;
      break;
    case 0x00C9:
    case 0x00E9:
      charIndex = SMALL_E;
      break;
    case 0x00CD:
    case 0x00ED:
      charIndex = SMALL_I;
      break;
    case 0x00D3:
    case 0x00F3:
      charIndex = SMALL_O;
      break;
    case 0x00D6:
    case 0x00F5:
    case 0x00F6:
    case 0x0150:
    case 0x0151:
      charIndex = SMALL_OO;
      break;
    case 0x00DA:
    case 0x00FA:
      charIndex = SMALL_U;
      break;
    case 0x0170:
    case 0x0171:
    case 0x00DC:
    case 0x00FC:
      charIndex = SMALL_UU;
      break;
    default:
    {
      if (charIndex == 255)
      {
        charIndex = 63; //?
      }
    }
    }
    newStr[index] = charIndex;
    str += addToStr;
    index++;
  } // end while

  newStr[index] = '\0';
  return newStr;
}