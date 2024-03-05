#include "DataSourceFrameRecorder.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

namespace DATA_SOURCE_TASK
{

// Function to find the nearest power of 2
size_t nearestPowerOfTwo(const size_t &n)
{
    if (n <= 1)
        return 1;

    return static_cast<size_t>(std::pow(2, std::ceil(std::log2(n))) / 2.);
}

DataSourceFrameRecorder::DataSourceFrameRecorder(const std::string & record_name, const int & num_elements):
    m_record_name {record_name}
{
    m_buffer_size = nearestPowerOfTwo(num_elements);

    m_frame_record[0].record_buffer.resize(m_buffer_size);
    m_frame_record[1].record_buffer.resize(m_buffer_size);
    m_frame_record[2].record_buffer.resize(m_buffer_size);

    m_frame_record[0].available_size = m_buffer_size;
    m_frame_record[1].available_size = m_buffer_size;
    m_frame_record[2].available_size = m_buffer_size;

    m_frame_record[0].id = 1;
    m_frame_record[1].id = 2;
    m_frame_record[2].id = 3;
}



void DataSourceFrameRecorder::recordBlock()
{
    std::ofstream source_file(m_record_name, std::ios::out | std::ios::binary /*| std::ios::app*/);

    for (std::size_t i = 0; i < MAX_REC_BUF_NUM; ++i)
    {
        struct record_buffer * buf = &m_frame_record[i];
        if (!buf->is_full)
            continue;

        source_file.write(reinterpret_cast<char *>(buf->record_buffer.data()), buf->record_buffer.size());

        if (source_file.fail())
        {
            std::ios_base::iostate state = source_file.rdstate();

            if (state & std::ios_base::eofbit)
            {
                std::cout << "DataSourceFrameRecorder: End of file reached." << std::endl;
            }
            if (state & std::ios_base::failbit)
            {
                std::cout << "DataSourceFrameRecorder: Non-fatal I/O error occurred." << std::endl;
            }
            if (state & std::ios_base::badbit)
            {
                std::cout << "DataSourceFrameRecorder: Fatal I/O error occurred." << std::endl;
            }
        }

        buf->is_full        = false;
        buf->pos            = 0;
        buf->available_size = m_buffer_size;
    }
}

void DataSourceFrameRecorder::updateBufs(
    std::shared_ptr<DataSourceBuffer<float>> frame, std::size_t availabale_in_data, int av_data_in_pos)
{
    for (std::size_t i = 0; i < MAX_REC_BUF_NUM; ++i)
    {
        struct record_buffer * buf = &m_frame_record[i];

        if (!buf->is_full)
        {
            std::size_t num_data_store = buf->available_size;

            if (availabale_in_data < num_data_store)
            {
                num_data_store = availabale_in_data;
            }

            memcpy(buf->record_buffer.data() + buf->pos, frame->payload() + av_data_in_pos, num_data_store);

            buf->pos += num_data_store;
            buf->available_size -= num_data_store;
            buf->is_full = (buf->pos >= m_buffer_size);

            availabale_in_data -= num_data_store;

            av_data_in_pos += availabale_in_data;
        }
    }
}

void DataSourceFrameRecorder::putNewFrame(std::shared_ptr<DataSourceBuffer<float>> frame)
{
    std::lock_guard<std::mutex> lock(m_write_lock);

    std::size_t availabale_in_data = frame->payloadSize();
    int av_data_in_pos             = 0;

    updateBufs(frame, availabale_in_data, av_data_in_pos);

    if (m_frame_record[0].is_full && m_frame_record[1].is_full && m_frame_record[2].is_full)
    {
        // Зіллємо буфера
        recordBlock();

        // Запишем залишок
        updateBufs(frame, availabale_in_data, av_data_in_pos);
    }
}

} // namespace DATA_SOURCE_TASK
