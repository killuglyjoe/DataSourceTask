#ifndef DATASOURCEFRAMERECORDER_H
#define DATASOURCEFRAMERECORDER_H

#include "DataSourceBuffer.h"

#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

namespace DATA_SOURCE_TASK
{
// К-сть буферів для обробки.
static constexpr std::size_t MAX_REC_BUF_NUM {2};

// К-сть блоків кратних степеню двійки для запису в файл.
static constexpr std::size_t RECORD_SIZE {10};

struct record_buffer
{
    int id;
    bool is_full                 = false; // готовність до запису в файл
    std::uint32_t pos            = 0;     // поточна позиція запису в буфер
    std::uint32_t available_size = 0;     // залишок елементів для перезапису
    std::vector<float> record_buffer;     // масив елементів
};

/// \brief Клас реалізовує функціонал складання і зберігання кадрів в файл.
/// -	складати результати обробки перерозподілити у блоки,
///     кількість відліків сигналу у яких є найближчим степенем двійки;
class DataSourceFrameRecorder
{
public:
    /// \brief Конструктор класу
    /// \param record_name - базове ім'я файлу зберігання
    /// \param block_size - к-сть елемнтів
    DataSourceFrameRecorder(const std::string & record_name, const int & num_elements);
    virtual ~DataSourceFrameRecorder();

    /// \brief К-сть відліків для запису, к-сть кратна степеню двійки.
    /// \return
    inline std::uint32_t bufferSize() const { return m_buffer_size; }

    /// \brief Заповнюємо буфери розміром до к-сті відліків степеня 2.
    /// Розмір вхідних даних менше ніж виділено під запис.
    /// \param buffer - оброблені дані float
    /// \param total_elements - к-сть відліків float
    void putNewFrame(const std::shared_ptr<DataSourceBuffer<float>> & buffer, const int & total_elements);

    /// \brief Замір часу на запис в файл.
    /// \return
    double elapsed() const { return m_elapsed; }

protected:
    /// \brief Асинхронний запис в файл.
    void recordBlock();

private:
    std::uint8_t m_active_buffer_index = 0;
    std::uint32_t m_buffer_size        = 0;        // к-сть відліків степепня числа 2
    std::string m_record_name          = "record"; // ім'я файлу.

    mutable std::atomic<bool> m_is_can_record_active; // Активатор потоку запису
    std::thread m_record_to_file;
    double m_elapsed = 0.;

    std::mutex m_buf_lock;
    std::atomic<bool> m_need_record;

    struct record_buffer m_frame_record[MAX_REC_BUF_NUM]; // масиви для заповнення float відліками даних.

    std::vector<float> m_record_buffer; // дані для запису в файл.
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEFRAMERECORDER_H
