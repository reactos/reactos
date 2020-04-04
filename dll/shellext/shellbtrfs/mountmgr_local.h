#pragma once

#include <vector>
#include <string>
#include <sstream>
#ifndef __REACTOS__
#include <string_view>
#else
#define string_view string
#define wstring_view wstring
#endif
#include <iostream>
#include <iomanip>

class mountmgr_point {
public:
    mountmgr_point(const std::wstring_view& symlink, const std::string_view& unique_id, const std::wstring_view& device_name) : symlink(symlink), device_name(device_name), unique_id(unique_id) {
    }

    std::wstring symlink, device_name;
    std::string unique_id;
};

class mountmgr {
public:
    mountmgr();
    ~mountmgr();
    void create_point(const std::wstring_view& symlink, const std::wstring_view& device) const;
    void delete_points(const std::wstring_view& symlink, const std::wstring_view& unique_id = L"", const std::wstring_view& device_name = L"") const;
    std::vector<mountmgr_point> query_points(const std::wstring_view& symlink = L"", const std::wstring_view& unique_id = L"", const std::wstring_view& device_name = L"") const;

private:
    HANDLE h;
};
