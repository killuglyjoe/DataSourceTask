#ifndef DATASOURCEFRAMERECORDER_H
#define DATASOURCEFRAMERECORDER_H

#include "DataSourceBuffer.h"

#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

namespace DATA_SOURCE_TASK
{

static constexpr std::size_t MAX_REC_BUF_NUM {3};

struct record_buffer
{
    int id;
    bool is_full       = false;       // готовність до запису в файл
    int pos            = 0;           // поточна позиція запису в буфер
    int available_size = 0;           // залишок елементів для перезапису
    std::vector<float> record_buffer; // масив елементів
};

/// \brief Клас реалізовує функціонал складання і зберігання кадрів d afqk
/// -	складати результати обробки перерозподілити у блоки,
///     кількість відліків сигналу у яких є найближчим степенем двійки;
class DataSourceFrameRecorder
{
public:
    /// \brief Конструктор класу
    /// \param record_name - базове ім'я файлу зберігання
    /// \param block_size - к-сть елемнтів
    DataSourceFrameRecorder(const std::string & record_name, const int & num_elements);
    virtual ~DataSourceFrameRecorder() {}

    /// \brief Заповнюємо буфери розміром до к-сті відліків степеня 2
    /// \param buffer - дані джерела
    void putNewFrame(const std::shared_ptr<DataSourceBuffer<float>> & buffer);

    /// \brief Замір часу на запис в буфери
    /// \return
    double elapsed() const { return m_elapsed; }

protected:
    /// \brief Асинхронний запис в файл
    void recordBlock();

    /// \brief перепаковування блоків даних блоки розміром степеня числа 2
    /// \param frame - вхідний кадр
    /// \param availabale_in_data - дані для запису вхідного кадру
    /// \param av_data_in_pos - зсув в данх
    void updateBufs(
        const std::shared_ptr<DataSourceBuffer<float>> & frame, std::size_t availabale_in_data, int av_data_in_pos);

private:
    std::uint8_t m_active_buffer_index = 0;
    int m_buffer_size                  = 0; // к-сть відліків степепня числа 2
    std::string m_record_name          = "record";

    std::thread record_to_file;
    double m_elapsed = 0.;

    std::mutex m_buf_lock;
    std::atomic<bool> m_need_record;

    struct record_buffer m_frame_record[3]; // буфери даних для запису в файл
    struct record_buffer m_tail_record;     // буфер для залишку
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEFRAMERECORDER_H
