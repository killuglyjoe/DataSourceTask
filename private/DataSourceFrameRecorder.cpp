#include "DataSourceFrameRecorder.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

namespace DATA_SOURCE_TASK
{

// Функція вертає найближчезначення числа степеня 2
size_t nearestPowerOfTwo(const size_t & n)
{
    if (n <= 1)
        return 1;

    return static_cast<size_t>(std::pow(2, std::ceil(std::log2(n))) / 2.);
}

// Активатор потоку запису
static std::atomic<bool> is_can_record_active;

DataSourceFrameRecorder::DataSourceFrameRecorder(const std::string & record_name,
                                                 const int & num_elements):
    m_record_name {record_name},
    m_need_record {false}
{
    m_buffer_size = nearestPowerOfTwo(num_elements);

    for (std::size_t i = 0; i < MAX_REC_BUF_NUM; ++i)
    {
        struct record_buffer * buf = &m_frame_record[i];
        buf->record_buffer.resize(m_buffer_size);
        buf->available_size = m_buffer_size;
        buf->id             = i + 1;
    }

    m_tail_record.record_buffer.resize(m_buffer_size);
    m_tail_record.available_size = m_buffer_size;
    m_tail_record.id             = 2;

    // асинхронний потік запису в файл
    record_to_file = std::thread(&DataSourceFrameRecorder::recordBlock, this);
}

void DataSourceFrameRecorder::recordBlock()
{
    is_can_record_active = true;

    while (is_can_record_active)
    {
        if (!m_need_record)
            continue;

        std::ofstream source_file(m_record_name, std::ios::out | std::ios::binary);

        if (!source_file)
        {
            for (std::size_t i = 0; i < MAX_REC_BUF_NUM; ++i)
            {
                struct record_buffer * buf = &m_frame_record[i];
                if (!buf->is_full)
                    continue;

                buf->is_full        = false;
                buf->pos            = 0;
                buf->available_size = m_buffer_size;
            }

            continue;
        }

        for (std::size_t i = 0; i < MAX_REC_BUF_NUM; ++i)
        {
            struct record_buffer * buf = &m_frame_record[i];
            if (!buf->is_full)
                continue;

            char * wbuf  = reinterpret_cast<char *>(buf->record_buffer.data());
            int buz_size = buf->record_buffer.size() * sizeof(float);

            source_file.write(wbuf, buz_size);

            m_need_record = false;

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
}

void DataSourceFrameRecorder::updateBufs(const std::shared_ptr<DataSourceBuffer<float>> & frame,
                                         std::size_t availabale_in_data,
                                         int av_data_in_pos)
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

            std::copy(frame->payload() + av_data_in_pos,
                      frame->payload() + av_data_in_pos + num_data_store,
                      buf->record_buffer.data() + buf->pos);

            buf->pos += num_data_store;
            buf->available_size -= num_data_store;
            buf->is_full = (buf->pos >= m_buffer_size);

            availabale_in_data -= num_data_store;

            av_data_in_pos += availabale_in_data;
        }
    }
}

void DataSourceFrameRecorder::putNewFrame(const std::shared_ptr<DataSourceBuffer<float>> & frame)
{
    std::lock_guard<std::mutex> lock(m_buf_lock);

    Timer timer;
    timer.reset();

    if (m_tail_record.is_full)
    {
        struct record_buffer * buf = &m_frame_record[0];
        buf->record_buffer.swap(m_tail_record.record_buffer);
        buf          = &m_tail_record;
        buf->is_full = false;
    }

    std::size_t availabale_in_data = frame->payloadSize();
    int av_data_in_pos             = 0;

    updateBufs(frame, availabale_in_data, av_data_in_pos);

    if (m_frame_record[0].is_full && m_frame_record[1].is_full && m_frame_record[2].is_full)
    {
        m_need_record = true;

        // Запишем залишок
        struct record_buffer * buf = &m_tail_record;
        std::size_t num_data_store = buf->available_size;

        if (availabale_in_data < num_data_store)
        {
            num_data_store = availabale_in_data;
        }
        buf->is_full = true;

        std::copy(frame->payload() + av_data_in_pos,
                  frame->payload() + av_data_in_pos + num_data_store,
                  buf->record_buffer.data() + buf->pos);
    }

    m_elapsed = timer.elapsed();
}

} // namespace DATA_SOURCE_TASK
