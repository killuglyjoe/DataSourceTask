#ifndef DATASOURCEEMULATOR_H
#define DATASOURCEEMULATOR_H

#include "DataSourceFile.h"
#include "DataSourceBuffer.h"

#include <fstream>
#include <random>
#include <thread>

namespace DATA_SOURCE_TASK
{

/// \brief Клас емулює роботу зовнішноього джарела даних.
/// В потоці йде запис в файл заданого кадру з данимим.
class DataSourceFileEmulator : public DataSource
{
public:
    /// \brief Емулятор певного джерела
    /// \param s_type - тип джерела
    /// \param p_type - тип даних
    /// \param num_elements - к-сть відліків сигналу
    DataSourceFileEmulator(
        const DATA_SOURCE_TASK::SOURCE_TYPE & s_type,
        const DATA_SOURCE_TASK::PAYLOAD_TYPE & p_type,
        const int & num_elements);

    virtual ~DataSourceFileEmulator();

    int read(char * data, int size) override;

protected:
    void updateData();

    void updateBufs();

private:
    int m_byte_size = 0;

    std::mt19937 m_mt;
    std::uniform_real_distribution<float> m_dist;

    std::thread m_write_thread;

    std::shared_ptr<DATA_SOURCE_TASK::DataSourceBufferInterface> m_buffer;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEEMULATOR_H
