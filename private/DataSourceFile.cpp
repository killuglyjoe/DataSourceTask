#include "DataSourceFile.h"

#include <fstream>
#include <iostream>

namespace DATA_SOURCE_TASK
{

DataSourceFile::DataSourceFile(const std::string & file_path):
    DataSource(),
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

    int index = 0;
    char character;

    while (index < size && (source_file.get(character)))
    {
        data[index++] = character;
    }

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

    return index;
}

} // namespace DATA_SOURCE_TASK
