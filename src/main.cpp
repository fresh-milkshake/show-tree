#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

struct Settings
{
    fs::path directory;
    int max_depth = -1;
    std::vector<std::string> filters;
    enum SortType
    {
        NAME,
        SIZE,
        DATE
    } sort_type = NAME;
    bool include_hidden = false;
};

bool is_hidden(const fs::directory_entry &entry)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesW(entry.path().c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return false;
    return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
#else
    return entry.path().filename().string().starts_with(".");
#endif
}

bool parse_arguments(int argc, char *argv[], Settings &settings)
{
    if (argc < 2)
    {
        std::cerr << "Ошибка: Необходимо указать путь к каталогу.\n";
        return false;
    }

    settings.directory = fs::path(argv[1]);

    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-d" || arg == "--depth") && i + 1 < argc)
        {
            settings.max_depth = std::stoi(argv[++i]);
        }
        else if ((arg == "-f" || arg == "--filter") && i + 1 < argc)
        {
            std::string filter_str = argv[++i];
            size_t pos = 0;
            while ((pos = filter_str.find(',')) != std::string::npos)
            {
                settings.filters.push_back(filter_str.substr(0, pos));
                filter_str.erase(0, pos + 1);
            }
            if (!filter_str.empty())
            {
                settings.filters.push_back(filter_str);
            }
        }
        else if ((arg == "-s" || arg == "--sort") && i + 1 < argc)
        {
            std::string sort_arg = argv[++i];
            if (sort_arg == "name")
            {
                settings.sort_type = Settings::NAME;
            }
            else if (sort_arg == "size")
            {
                settings.sort_type = Settings::SIZE;
            }
            else if (sort_arg == "date")
            {
                settings.sort_type = Settings::DATE;
            }
            else
            {
                std::cerr << "Неверный параметр сортировки: " << sort_arg << "\n";
                return false;
            }
        }
        else if ((arg == "-h" || arg == "--hidden"))
        {
            settings.include_hidden = true;
        }
        else
        {
            std::cerr << "Неизвестный параметр: " << arg << "\n";
            return false;
        }
    }

    return true;
}

void sort_directory(std::vector<fs::directory_entry> &entries, Settings::SortType sort_type)
{
    auto file_type_sort = [](const fs::directory_entry &a, const fs::directory_entry &b) -> bool
    {
        if (a.is_directory() != b.is_directory())
        {
            return a.is_directory() > b.is_directory();
        }

        std::string ext_a = a.path().extension().string();
        std::string ext_b = b.path().extension().string();
        if (ext_a != ext_b)
        {
            return ext_a < ext_b;
        }

        return a.path().filename().string() < b.path().filename().string();
    };

    switch (sort_type)
    {
    case Settings::SortType::NAME:
        std::sort(entries.begin(), entries.end(), file_type_sort);
        break;
    case Settings::SortType::SIZE:
        std::sort(entries.begin(), entries.end(),
                  [file_type_sort](const fs::directory_entry &a, const fs::directory_entry &b) -> bool
                  {
                      std::error_code ec1, ec2;
                      auto size_a = a.is_directory() ? 0 : a.file_size(ec1);
                      auto size_b = b.is_directory() ? 0 : b.file_size(ec2);
                      if (size_a != size_b)
                          return size_a < size_b;
                      return file_type_sort(a, b);
                  });
        break;
    case Settings::SortType::DATE:
        std::sort(entries.begin(), entries.end(),
                  [file_type_sort](const fs::directory_entry &a, const fs::directory_entry &b) -> bool
                  {
                      std::error_code ec1, ec2;
                      auto time_a = a.last_write_time(ec1).time_since_epoch().count();
                      auto time_b = b.last_write_time(ec2).time_since_epoch().count();
                      if (time_a != time_b)
                          return time_a < time_b;
                      return file_type_sort(a, b);
                  });
        break;
    }
}

bool matches_filter(const fs::directory_entry &entry, const Settings &settings)
{
    if (settings.filters.empty())
        return true;
    if (entry.is_directory())
        return false;
    std::string ext = entry.path().extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (const auto &f : settings.filters)
    {
        std::string filter = f;
        if (filter.front() != '.')
        {
            filter = "." + filter;
        }
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
        if (ext == filter)
            return true;
    }
    return false;
}

void print_tree(const fs::path &path, const Settings &settings, int depth = 0, const std::string &prefix = "")
{
    if (settings.max_depth != -1 && depth > settings.max_depth)
        return;

    std::vector<fs::directory_entry> entries;
    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_directory(path, ec))
    {
        std::cerr << "Неверный путь: " << path << "\n";
        return;
    }

    for (const auto &entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
            continue;
        if (!settings.include_hidden && is_hidden(entry))
            continue;
        if (!matches_filter(entry, settings))
            continue;
        entries.push_back(entry);
    }

    sort_directory(entries, settings.sort_type);

    size_t count = entries.size();
    for (size_t i = 0; i < count; ++i)
    {
        const auto &entry = entries[i];
        bool is_last = (i == count - 1);
        std::cout << prefix;
        std::cout << (is_last ? "└─ " : "├─ ");

        std::cout << entry.path().filename().string();

        if (entry.is_directory())
        {
            std::cout << "/";
        }

        std::cout << "\n";

        if (entry.is_directory())
        {
            std::string new_prefix = prefix + (is_last ? "    " : "│   ");
            print_tree(entry.path(), settings, depth + 1, new_prefix);
        }
    }
}

int main(int argc, char *argv[])
{
    Settings settings;
    if (!parse_arguments(argc, argv, settings))
    {
        std::cout << "Использование:\n";
        std::cout << argv[0] << " <путь_к_каталогу> [опции]\n";
        std::cout << "Опции:\n";
        std::cout << "  -d, --depth <число>        Глубина рекурсии\n";
        std::cout << "  -f, --filter <расширения>  Фильтр типов файлов (через запятую)\n";
        std::cout << "  -s, --sort <тип>           Сортировка: name, size, date\n";
        std::cout << "  -h, --hidden               Показывать скрытые файлы и папки\n";
        return 1;
    }

    print_tree(settings.directory, settings);

    return 0;
}
