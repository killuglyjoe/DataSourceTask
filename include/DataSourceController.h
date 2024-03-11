#ifndef DATASOURCECONTROLLER_H
#define DATASOURCECONTROLLER_H

#include "DataSource.h"
#include "DataSourceBuffer.h"
#include "DataSourceFrameProcessor.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace DATA_SOURCE_TASK
{

/// \brief Клас контролер для конкретного типу джерела.
class DataSourceController
{
public:
    /// \brief Конструктор класу. Тут виділомо память під клас DataSource
    /// і відповідно пам'ять для кадру
    /// \param source_path - безпосередньо походження джерела (шлях до файлу, мережева адреса тощо). Може треба
    /// параметризувати цей параметр \param source_type - тип джереала \param p_type - тип корисних даних \param
    /// frame_size - к-сть елементів в payload
    DataSourceController(const std::string & source_path,
                         const SOURCE_TYPE & source_type,
                         const PAYLOAD_TYPE & p_type,
                         const std::uint32_t & frame_size);

    virtual ~DataSourceController();

    inline int header() { return m_buffer[m_active_buffer]->header(); }

    inline int getPacketsLoss() const { return m_data_source_frm_processor->getPacketsLoss(); }
    inline int getBadFrames() const { return m_data_source_frm_processor->getBadFrames(); }

    inline int framesTotal() { return m_buffer[m_active_buffer]->frameCounter(); }

    inline double elapsed() { return m_elapsed; }
    inline double writeFramelapsed() { return m_data_source->elapsed(); }
    inline double saveFramelapsed() { return m_data_source_frm_processor->saveFrameElapsed(); }
    inline double frameValidationElapsed() { return m_data_source_frm_processor->elapsed(); }

protected:
    void readData();

private:
    int m_payload_byte_size;

    std::atomic<int> m_elapsed;

    std::unique_ptr<DataSource> m_data_source;
    std::unique_ptr<DataSourceFrameProcessor> m_data_source_frm_processor;
    std::shared_ptr<DataSourceBufferInterface> m_buffer[MAX_PROCESSING_BUF_NUM];

    std::atomic<int> m_active_buffer; // поточний буфер для обробки
    std::mutex m_mutex;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCECONTROLLER_H
