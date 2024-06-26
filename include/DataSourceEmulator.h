#ifndef DATASOURCEEMULATOR_H
#define DATASOURCEEMULATOR_H

#include "DataSource.h"
#include "DataSourceBuffer.h"

#include <mutex>
#include <memory>

namespace DATA_SOURCE_TASK
{

/// \brief Клас емулює роботу зовнішноього джарела даних.
class DataSourceFileEmulator final : public DataSource
{
public:
    /// \brief Емулятор певного джерела
    /// \param s_type - тип джерела
    /// \param p_type - тип даних
    /// \param frame_size - к-сть відліків сигналу
    explicit DataSourceFileEmulator(const DATA_SOURCE_TASK::PAYLOAD_TYPE & p_type, const int & frame_size);

    virtual ~DataSourceFileEmulator();

    int read(char * data, int size) override;

protected:
    void updateData();

    void updateBufs();

    void generateRandom();

private:
    int m_byte_size = 0;

    std::atomic<uint16_t> m_frm_counter {0};
    std::mutex m_read_lock;

    std::shared_ptr<DataSourceBufferInterface> m_buffer;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEEMULATOR_H
