#include "utils.h"

#include <cmath>
#include <string>

#include "app.h"

int Utils::GetTextLength(const char *text)
{
	if (text == nullptr)
		return 0;

	for (auto i = 0; i < 0x7FFF; i++)
	{
		if (text[i] == '\0')
			return i;
	}

	return 0;
}

void Utils::ValidateInt(int& value, const int min, const int max)
{
	if (value < min)
		value = min;
	else if (value > max)
		value = max;
}

void Utils::ValidateFloat(float& value, const float min, const float max)
{
	if (value < min)
		value = min;
	else if (value > max)
		value = max;
}

void Utils::ValidateRgb(float *value)
{
	for (auto i = 0; i < 3; i++)
	{
		ValidateFloat(value[i], 0.0f, 1.0f);
	}
}

bool Utils::Valid(const DWORD64 ptr)
{
	if (ptr < 0x10000UL || ptr > 0xF000000000000ULL || ptr & 1)
		return false;
	
	return true;
}

float Utils::GetDistance(const float *a1, const float *a2)
{
	return sqrtf(powf(a1[0] - a2[0], 2.0f) + powf(a1[1] - a2[1], 2.0f) + powf(a1[2] - a2[2], 2.0f));
}

void Utils::ProjectView(float *dst, const float *forward, const float *origin, const float distance)
{
	dst[0] = origin[0] + forward[0] * distance;
	dst[1] = origin[1] + forward[1] * distance;
	dst[2] = origin[2] + forward[2] * distance;
}

float Utils::RadiansToDegrees(const float radians)
{
	return radians * (180.0f / 3.14159265f);
}

float Utils::GetDegrees(float *src, float *forward, float *origin)
{
	float buffer[3];
	ProjectView(buffer, forward, origin, GetDistance(src, origin));
	return RadiansToDegrees(sinf(GetDistance(src, buffer) / GetDistance(origin, buffer)));
}

bool Utils::WorldToScreen(const float *view, const float *position, float *screen)
{
	float buffer[4];
	buffer[0] = view[0] * position[0] + -view[1] * -position[1] + view[2] * position[2] + view[3];
	buffer[1] = view[4] * position[0] + -view[5] * -position[1] + view[6] * position[2] + view[7];
	//Buffer[2] = View[8] * Position[0] + -View[9] * -Position[1] + View[10] * Position[2] + View[11];
	buffer[3] = view[12] * position[0] + -view[13] * -position[1] + view[14] * position[2] + view[15];
	if (buffer[3] < 0.1f) return false;

	float halfWindowSize[2];
	halfWindowSize[0] = static_cast<float>(App::windowSize[0]) * 0.5f;
	halfWindowSize[1] = static_cast<float>(App::windowSize[1]) * 0.5f;

	screen[0] = halfWindowSize[0] + halfWindowSize[0] * buffer[0] / buffer[3];
	screen[1] = halfWindowSize[1] - halfWindowSize[1] * buffer[1] / buffer[3];
	return true;
}

int Utils::GetRangedInt(const int min, const int max)
{
	if (min < max)
		return rand() % (1 + (max - min)) + (max - (max - min));
	
	return min;
}

std::string Utils::UTF8ToGBK(const char* strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}
