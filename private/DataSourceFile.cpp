#include "DataSourceFile.h"

#include <fstream>
#include <iostream>

namespace DATA_SOURCE_TASK
{

DataSourceFile::DataSourceFile(const std::string & file_path):
    DataSource(SOURCE_TYPE::SOURCE_TYPE_FILE),
    m_file_path {file_path}
{
}

DataSourceFile::~DataSourceFile() {}

int DataSourceFile::read(char * data, int size)
{
    std::lock_guard<std::mutex> lock(m_data_mutex);

    std::ifstream source_file(m_file_path, std::ios::in | std::ios::binary);

    if (!source_file)
        return static_cast<int>(DATA_SOURCE_ERROR::READ_SOURCE_ERROR);

    if (!source_file.seekg(0, std::ios::beg))
        return static_cast<int>(DATA_SOURCE_ERROR::READ_SOURCE_ERROR);

    source_file.read(data, size);

    if (source_file.fail())
    {
        std::ios_base::iostate state = source_file.rdstate();

        if (state & std::ios_base::eofbit)
        {
            std::cout << "DataSourceFile End of file reached." << std::endl;
        }
        if (state & std::ios_base::failbit)
        {
            std::cout << "DataSourceFile Non-fatal I/O error occurred." << std::endl;
        }
        if (state & std::ios_base::badbit)
        {
            std::cout << "DataSourceFile Fatal I/O error occurred." << std::endl;
        }

        return static_cast<int>(DATA_SOURCE_ERROR::READ_SOURCE_ERROR);
    }

    return size;
}

} // namespace DATA_SOURCE_TASK
