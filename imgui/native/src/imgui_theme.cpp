#include "imgui_theme.h"
#include "../third_party/imgui/imgui.h"
#include <windows.h>
#include <cstdio>
#include <cstring>

struct ColorField {
	ImGuiCol col;
	const char *name;
	float r, g, b, a;
};

static const ColorField kColorFields[] = {
	{ImGuiCol_WindowBg, "WindowBg", 0.12f, 0.13f, 0.15f, 0.35f},
	{ImGuiCol_ChildBg, "ChildBg", 0.14f, 0.15f, 0.17f, 0.35f},
	{ImGuiCol_PopupBg, "PopupBg", 0.10f, 0.10f, 0.12f, 0.35f},
	{ImGuiCol_Border, "Border", 0.30f, 0.33f, 0.42f, 0.25f},
	{ImGuiCol_Text, "Text", 0.90f, 0.93f, 0.95f, 1.00f},
	{ImGuiCol_TextDisabled, "TextDisabled", 0.60f, 0.65f, 0.70f, 1.00f},
	{ImGuiCol_Header, "Header", 0.36f, 0.42f, 0.55f, 0.35f},
	{ImGuiCol_HeaderHovered, "HeaderHovered", 0.44f, 0.50f, 0.68f, 0.35f},
	{ImGuiCol_HeaderActive, "HeaderActive", 0.46f, 0.55f, 0.75f, 0.35f},
	{ImGuiCol_Button, "Button", 0.28f, 0.34f, 0.48f, 0.70f},
	{ImGuiCol_ButtonHovered, "ButtonHovered", 0.36f, 0.45f, 0.65f, 0.85f},
	{ImGuiCol_ButtonActive, "ButtonActive", 0.40f, 0.50f, 0.70f, 1.00f},
	{ImGuiCol_FrameBg, "FrameBg", 0.20f, 0.22f, 0.28f, 0.35f},
	{ImGuiCol_FrameBgHovered, "FrameBgHovered", 0.28f, 0.32f, 0.42f, 1.00f},
	{ImGuiCol_FrameBgActive, "FrameBgActive", 0.32f, 0.38f, 0.50f, 1.00f},
	{ImGuiCol_PlotHistogram, "PlotHistogram", 0.46f, 0.55f, 0.75f, 0.35f},
	{ImGuiCol_PlotHistogramHovered, "PlotHistogramHovered", 0.55f, 0.64f, 0.85f, 0.35f},
	{ImGuiCol_Tab, "Tab", 0.26f, 0.30f, 0.42f, 0.80f},
	{ImGuiCol_TabHovered, "TabHovered", 0.36f, 0.42f, 0.58f, 1.00f},
	{ImGuiCol_TabSelected, "TabSelected", 0.42f, 0.50f, 0.68f, 1.00f},
	{ImGuiCol_TabDimmed, "TabDimmed", 0.20f, 0.24f, 0.32f, 0.80f},
	{ImGuiCol_TabDimmedSelected, "TabDimmedSelected", 0.30f, 0.36f, 0.50f, 1.00f},
	{ImGuiCol_TitleBg, "TitleBg", 0.20f, 0.25f, 0.30f, 0.35f},
	{ImGuiCol_TitleBgActive, "TitleBgActive", 0.25f, 0.30f, 0.40f, 0.35f},
	{ImGuiCol_TitleBgCollapsed, "TitleBgCollapsed", 0.10f, 0.12f, 0.15f, 0.35f},
	{ImGuiCol_TableHeaderBg, "TableHeaderBg", 0.20f, 0.25f, 0.30f, 0.35f},
	{ImGuiCol_ScrollbarBg, "ScrollbarBg", 0.13f, 0.14f, 0.18f, 1.00f},
	{ImGuiCol_ScrollbarGrab, "ScrollbarGrab", 0.25f, 0.30f, 0.38f, 0.60f},
	{ImGuiCol_ScrollbarGrabHovered, "ScrollbarGrabHovered", 0.35f, 0.40f, 0.50f, 0.80f},
	{ImGuiCol_ScrollbarGrabActive, "ScrollbarGrabActive", 0.45f, 0.50f, 0.65f, 1.00f},
	{ImGuiCol_CheckMark, "CheckMark", 0.80f, 0.85f, 1.00f, 1.00f},
	{ImGuiCol_SliderGrab, "SliderGrab", 0.50f, 0.65f, 0.90f, 1.00f},
	{ImGuiCol_SliderGrabActive, "SliderGrabActive", 0.60f, 0.75f, 1.00f, 1.00f},
	{ImGuiCol_ResizeGrip, "ResizeGrip", 0.30f, 0.40f, 0.50f, 0.60f},
	{ImGuiCol_ResizeGripHovered, "ResizeGripHovered", 0.40f, 0.50f, 0.60f, 0.80f},
	{ImGuiCol_ResizeGripActive, "ResizeGripActive", 0.50f, 0.60f, 0.80f, 1.00f},
	{ImGuiCol_Separator, "Separator", 0.35f, 0.40f, 0.48f, 0.7f},
	{ImGuiCol_SeparatorHovered, "SeparatorHovered", 0.50f, 0.60f, 0.72f, 0.9f},
	{ImGuiCol_SeparatorActive, "SeparatorActive", 0.65f, 0.70f, 0.85f, 1.0f},
	{ImGuiCol_MenuBarBg, "MenuBarBg", 0.14f, 0.15f, 0.17f, 1.00f},
	{ImGuiCol_DragDropTarget, "DragDropTarget", 0.50f, 0.85f, 1.00f, 0.90f},
};

struct FloatField {
	const char *name;
	float ImGuiStyle::*member;
	float def;
};

static const FloatField kFloatFields[] = {
	{"WindowRounding", &ImGuiStyle::WindowRounding, 8.0f},
	{"ChildRounding", &ImGuiStyle::ChildRounding, 6.0f},
	{"FrameRounding", &ImGuiStyle::FrameRounding, 5.0f},
	{"PopupRounding", &ImGuiStyle::PopupRounding, 6.0f},
	{"ScrollbarRounding", &ImGuiStyle::ScrollbarRounding, 5.0f},
	{"GrabRounding", &ImGuiStyle::GrabRounding, 4.0f},
	{"TabRounding", &ImGuiStyle::TabRounding, 5.0f},
	{"WindowBorderSize", &ImGuiStyle::WindowBorderSize, 0.0f},
	{"FrameBorderSize", &ImGuiStyle::FrameBorderSize, 0.0f},
	{"PopupBorderSize", &ImGuiStyle::PopupBorderSize, 1.0f},
	{"IndentSpacing", &ImGuiStyle::IndentSpacing, 20.0f},
};

struct Vec2Field {
	const char *name;
	ImVec2 ImGuiStyle::*member;
	float x, y;
};

static const Vec2Field kVec2Fields[] = {
	{"WindowPadding", &ImGuiStyle::WindowPadding, 16.0f, 16.0f},
	{"FramePadding", &ImGuiStyle::FramePadding, 10.0f, 6.0f},
	{"ItemSpacing", &ImGuiStyle::ItemSpacing, 10.0f, 10.0f},
	{"ItemInnerSpacing", &ImGuiStyle::ItemInnerSpacing, 6.0f, 4.0f},
};

void ApplyDefaultTheme() {
	ImGuiStyle &style = ImGui::GetStyle();
	for (auto &f : kColorFields) style.Colors[f.col] = ImVec4(f.r, f.g, f.b, f.a);
	for (auto &f : kFloatFields) style.*f.member = f.def;
	for (auto &f : kVec2Fields) style.*f.member = ImVec2(f.x, f.y);
}

static void GetThemeConfigPath(char *outPath, size_t outPathSize) {
	char dir[MAX_PATH];
	GetModuleFileNameA(NULL, dir, MAX_PATH);
	char *slash = strrchr(dir, '\\');
	if (slash) slash[1] = 0; else dir[0] = 0;
	strcat_s(dir, MAX_PATH, "hlx");
	CreateDirectoryA(dir, NULL);
	strcat_s(dir, MAX_PATH, "\\config");
	CreateDirectoryA(dir, NULL);
	strcat_s(dir, MAX_PATH, "\\imgui");
	CreateDirectoryA(dir, NULL);
	strcpy_s(outPath, outPathSize, dir);
	strcat_s(outPath, outPathSize, "\\theme.conf");
}

static void WriteThemeConfig(const char *path) {
	ImGuiStyle &style = ImGui::GetStyle();
	char buf[8192];
	int pos = 0;
	for (auto &f : kColorFields) {
		ImVec4 &c = style.Colors[f.col];
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%s=%f,%f,%f,%f\r\n", f.name, c.x, c.y, c.z, c.w);
	}
	for (auto &f : kFloatFields)
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%s=%f\r\n", f.name, style.*f.member);
	for (auto &f : kVec2Fields) {
		ImVec2 &v = style.*f.member;
		pos += sprintf_s(buf + pos, sizeof(buf) - pos, "%s=%f,%f\r\n", f.name, v.x, v.y);
	}

	HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return;
	DWORD written;
	WriteFile(h, buf, (DWORD)pos, &written, NULL);
	CloseHandle(h);
}

static bool TryParseColor(const char *key, const char *value) {
	for (auto &f : kColorFields)
		if (strcmp(key, f.name) == 0) {
			ImVec4 &c = ImGui::GetStyle().Colors[f.col];
			sscanf_s(value, "%f,%f,%f,%f", &c.x, &c.y, &c.z, &c.w);
			return true;
		}
	return false;
}

static bool TryParseFloat(const char *key, const char *value) {
	ImGuiStyle &style = ImGui::GetStyle();
	for (auto &f : kFloatFields)
		if (strcmp(key, f.name) == 0) {
			sscanf_s(value, "%f", &(style.*f.member));
			return true;
		}
	return false;
}

static bool TryParseVec2(const char *key, const char *value) {
	ImGuiStyle &style = ImGui::GetStyle();
	for (auto &f : kVec2Fields)
		if (strcmp(key, f.name) == 0) {
			ImVec2 &v = style.*f.member;
			sscanf_s(value, "%f,%f", &v.x, &v.y);
			return true;
		}
	return false;
}

void LoadOrInitThemeConfig() {
	char path[MAX_PATH];
	GetThemeConfigPath(path, MAX_PATH);

	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		WriteThemeConfig(path);
		return;
	}

	char buf[8192];
	DWORD readBytes = 0;
	ReadFile(h, buf, sizeof(buf) - 1, &readBytes, NULL);
	buf[readBytes] = 0;
	CloseHandle(h);

	char *bufEnd = buf + readBytes;
	char *lineEnd = NULL;
	for (char *line = buf; line < bufEnd; line = lineEnd + 1) {
		while (*line == '\n' || *line == '\r') line++;
		lineEnd = line;
		while (lineEnd < bufEnd && *lineEnd != '\n' && *lineEnd != '\r') lineEnd++;
		*lineEnd = 0;

		char *eq = strchr(line, '=');
		if (!eq) continue;
		*eq = 0;
		const char *value = eq + 1;

		if (!TryParseColor(line, value) && !TryParseFloat(line, value)) TryParseVec2(line, value);
	}
}
