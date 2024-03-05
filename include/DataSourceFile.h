#ifndef DATASOURCEFILE_H
#define DATASOURCEFILE_H

#include "DataSource.h"

#include <mutex>

namespace DATA_SOURCE_TASK
{

class DataSourceFile final : public DataSource
{
public:
    explicit DataSourceFile(const std::string & file_path);
    virtual ~DataSourceFile();

    int read(char * data, int size);

private:
    std::mutex m_data_mutex;
    std::string m_file_path;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEFILE_H
