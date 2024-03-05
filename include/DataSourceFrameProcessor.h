#ifndef DATASOURCEFRAMEPROCESSOR_H
#define DATASOURCEFRAMEPROCESSOR_H

#include "DataSourceBuffer.h"
#include <memory>
#include <mutex>

namespace DATA_SOURCE_TASK
{

enum class DATA_SOURCE_HW_CONV_TYPE : int
{
    DATA_SOURCE_HW_CONV_TYPE_CPU = 0,
    DATA_SOURCE_HW_CONV_TYPE_GPU
};

/// \brief Клас для валідації отриманого кадру з джерела даних.
/// Робить перевірку і складання кадрів.
class DataSourceFrameProcessor
{
public:
    /// \brief Клас для роботи з отриманимим кадрами.
    DataSourceFrameProcessor(const int & frame_size, const int & num_elements);
    virtual ~DataSourceFrameProcessor() {}

    /// \brief Перевірка бракованих кадрів
    /// \param frm
    /// \param updated_size
    bool validateFrame(const std::shared_ptr<DataSourceBufferInterface> & buffer, const int & updated_size);

    inline int frameSize() const { return m_frame_size; }

    inline int getPacketsLoss() const { return m_packets_loss; }

    std::shared_ptr<DataSourceBuffer<float>> curProcessedFrame() const  { return m_buffer; }

    inline double elapsed() { return m_elapsed; }

private:
    /// \brief перетворення масиву цілих чисел в float[]
    /// \param payload - масив цілих чисел
    void convertToFLoat(char * payload);

private:
    int m_frame_size   = 0;
    int m_packets_loss = 0;
    double m_elapsed   = 0;
    std::mutex m_process_mutex;
    std::shared_ptr<DataSourceBuffer<float>> m_buffer;  // дані будуть перетворені в float
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEFRAMEPROCESSOR_H
