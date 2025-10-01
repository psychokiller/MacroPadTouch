#ifndef ICONLOADER_H
#define ICONLOADER_H

#include <string>
#include <vector>

class IconLoader {
public:
    IconLoader();
    ~IconLoader();
    const unsigned char* loadIcon(const std::string& iconPath);

private:
    std::vector<const unsigned char*> loadedIcons;
};

#endif