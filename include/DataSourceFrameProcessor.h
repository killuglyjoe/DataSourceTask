#ifndef DATASOURCEFRAMEPROCESSOR_H
#define DATASOURCEFRAMEPROCESSOR_H

#include "DataSourceBuffer.h"
#include "DataSourceFrameRecorder.h"

#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace DATA_SOURCE_TASK
{

enum class DATA_SOURCE_HW_CONV_TYPE : int
{
    DATA_SOURCE_HW_CONV_TYPE_CPU = 0,
    DATA_SOURCE_HW_CONV_TYPE_GPU
};

// Буферізація масивів вхідних даних
static constexpr int BUFERIZATION_NUM {2};

/// \brief Клас для валідації отриманого кадру з джерела даних.
/// Робить перевірку і складання кадрів.
/// Заповнює буфери масивів даних, розмірністю MxN. К-сть буферів BUFERIZATION_NUM,
/// а вних к-сть кадрів MAX_PROCESSING_BUF_NUM.
class DataSourceFrameProcessor
{
public:
    /// \brief Клас для роботи з отриманимим кадрами.
    DataSourceFrameProcessor(const int & frame_size);
    virtual ~DataSourceFrameProcessor();

    /// \brief Перевірка бракованих кадрів.
    /// Конвертація в float.
    /// \param buffer - дані з джерела
    /// \return - к-сть rjydthnjdfyb[ відліків float
    int validateFrame(const std::shared_ptr<DataSourceBufferInterface> & buffer);
    /// \brief Розмір кадру
    /// \return
    inline int frameSize() const { return m_frame_size; }
    /// \brief К-сть втрачених пакетів, рахуються по лячильнику в заголовку кадру
    /// \return
    inline int getPacketsLoss() const { return m_packets_loss; }
    /// \brief Браковані кадри.
    /// Рахуються по результату методу read, якщо розмір прочитаних даних менше бажаного.
    /// \return
    inline int getBadFrames() const { return m_bad_frames; }
    /// \brief К-сть кадрів з проблемами цілісності даних.
    /// \return
    inline int getBrokenFrames() const { return m_stream_broken; }
    /// \brief Функція записує вх. кадр в масив буферів кадрів.
    /// \param frame
    /// \param updated_size
    void putNewFrame(std::shared_ptr<DataSourceBufferInterface> & frame, int updated_size);
    /// \brief Пройдений час на обробки вх. даних в потоці.
    /// \return мілісекунди
    inline double elapsed() { return m_elapsed; }
    /// \brief Усереднений час запису оброблених даних в файл.
    /// \return мілісекунди
    double saveFrameElapsed();

protected:
    /// \brief Потокова функція обробки вхідних буферів
    void frameProcess();

private:
    /// \brief перетворення масиву цілих чисел в float[]
    /// \param payload - масив цілих чисел
    void convertToFLoat(char * payload);

private:
    int m_frame_size    = 0; // відомий розмір кадру
    int m_packets_loss  = 0; // втрати пакетів на основі лфчильника кадрів
    int m_stream_broken = 0; // потік даних не цілісний. Не вистачає байтів для даних.
    int m_bad_frames    = 0; // поганий пакет на основі повернутого розміру кадру

    double m_elapsed = 0;   // час обробки вх. даних, мс

    mutable std::mutex m_process_mutex;

    std::atomic<bool> m_can_validate;
    std::atomic<int> m_req_size;

    std::thread m_process_thread;
    std::atomic<bool> m_is_process_active;

    // --------------   Дані з джерела   --------------------
    std::atomic<int> m_src_ready_buffer; // 0...MAX_PROCESSING_BUF_NUM-1
    std::atomic<int> m_active_buffer;    // 0...BUFERIZATION_NUM-1

    // Два буфери - один наповнюємо з іншим працюємо. Потім навпаки
    std::shared_ptr<DataSourceBufferInterface> m_source_buffer[BUFERIZATION_NUM]
                                                              [MAX_PROCESSING_BUF_NUM]; // дані для swap з джерела

    // --------------   Оброблені дані (float)   --------------------
    std::atomic<int> m_flt_ready_buffer;                            // 0..MAX_PROCESSING_BUF_NUM-1
    std::vector<std::shared_ptr<DataSourceBuffer<float>>> m_buffer; // дані будуть перетворені в float

    // Реєстратор відліків блоками відліків, к-сть яких є число степеня 2.
    std::unordered_map<int, std::shared_ptr<DataSourceFrameRecorder> > m_data_source_frame_recorders;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEFRAMEPROCESSOR_H
