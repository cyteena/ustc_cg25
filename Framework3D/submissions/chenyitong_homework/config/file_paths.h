#pragma once

#include <string>
#include <filesystem>

namespace config {

// 获取正确的文件路径
inline std::string get_correct_file_path(const std::string& filename) {
    // 默认数据路径
    std::string base_path = "D:\\ustc_cg25\\Framework3D\\submissions\\chenyitong_homework\\data\\data_hw4\\meshes\\";
    
    // 如果提供了完整路径，则直接使用
    if (filename.find(":\\") != std::string::npos) {
        return filename;
    }
    
    // 检查默认路径中是否存在文件
    std::filesystem::path full_path = base_path + filename;
    if (std::filesystem::exists(full_path)) {
        return full_path.string();
    }
    
    // 尝试在可能的替代路径中查找
    std::vector<std::string> alternative_paths = {
        "D:\\ustc_cg25\\Framework3D\\Framework3D\\Binaries\\Release\\",
        "D:\\ustc_cg25\\Framework3D\\submissions\\chenyitong_homework\\data\\",
        ".\\data\\data_hw4\\meshes\\"
    };
    
    for (const auto& path : alternative_paths) {
        full_path = path + filename;
        if (std::filesystem::exists(full_path)) {
            return full_path.string();
        }
    }
    
    // 返回原始文件名，让系统尝试默认路径
    return filename;
}

}  // namespace config
