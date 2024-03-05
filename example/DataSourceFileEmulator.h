#ifndef DATASOURCEFILEEMULATOR_H
#define DATASOURCEFILEEMULATOR_H

#include "DataSourceBuffer.h"

#include <fstream>
#include <thread>

/// \brief Клас емулює роботу зовнішноього джарела даних.
/// В потоці йде запис в файл заданого кадру з данимим.
class DataSourceFileEmulator
{
public:
    /// \brief DataSourceFileEmulator
    /// \param file_path - назва файлу
    /// \param s_type - тип джерела
    /// \param p_type - тип даних
    /// \param num_elements - к-сть відліків сигналу
    DataSourceFileEmulator(
        const std::string & file_path,
        const DATA_SOURCE_TASK::SOURCE_TYPE & s_type,
        const DATA_SOURCE_TASK::PAYLOAD_TYPE & p_type,
        const int & num_elements);

    virtual ~DataSourceFileEmulator();

private:
    void writeData();

private:
    int m_byte_size = 0;
    std::ofstream m_source_file;
    std::string m_file_path;
    std::thread m_write_thread;
    std::shared_ptr<DATA_SOURCE_TASK::DataSourceBufferInterface> m_buffer;
};

#endif // DATASOURCEFILEEMULATOR_H
