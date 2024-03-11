#ifndef DATASOURCEBUFFER_H
#define DATASOURCEBUFFER_H

#include "globals.h"

#include <vector>

namespace DATA_SOURCE_TASK
{
class DataSourceBufferInterface
{
public:
    DataSourceBufferInterface() {}

    DataSourceBufferInterface & operator=(DataSourceBufferInterface & other) noexcept
    {
        buffer.swap(other.buffer);

        m_frame   = reinterpret_cast<struct frame *>(buffer.data());
        m_payload = reinterpret_cast<char *>(buffer.data() + FRAME_HEADER_SIZE);

        return *this;
    }

    DataSourceBufferInterface(DataSourceBufferInterface & other) noexcept
    {
        buffer.swap(other.buffer);
        m_frame   = reinterpret_cast<struct frame *>(buffer.data());
        m_payload = reinterpret_cast<char *>(buffer.data() + FRAME_HEADER_SIZE);
    }

    DataSourceBufferInterface(DataSourceBufferInterface && other) noexcept
    {
        buffer.swap(other.buffer);
        m_frame   = reinterpret_cast<struct frame *>(buffer.data());
        m_payload = reinterpret_cast<char *>(buffer.data() + FRAME_HEADER_SIZE);
    }

    virtual ~DataSourceBufferInterface() = default;

    inline struct frame * frame() { return m_frame; };

    /// \brief Функція повертає заголовок фрейму
    /// \return
    inline std::uint32_t header() const
    {
        if (m_frame)
            return m_frame->magic_word;

        return -1;
    }
    /// \brief Задамо заголовок кадру
    /// \return
    void setHeader(const std::uint32_t & header)
    {
        if (m_frame)
            m_frame->magic_word = header;
    }
    /// \brief Лічильник кадрів
    /// \return
    inline std::uint16_t frameCounter() const
    {
        if (m_frame)
            return m_frame->frame_counter;

        return -1;
    }
    /// \brief Задамо лічильник кадрів
    /// \return
    void setFrameCounter(const std::uint16_t & cntr)
    {
        if (m_frame)
            m_frame->frame_counter = cntr;
    }
    /// \brief ІД джерела
    /// \return
    inline SOURCE_TYPE sourceId() const
    {
        if (m_frame)
            return m_frame->source_id;

        return SOURCE_TYPE::SOURCE_TYPE_UNDEFINED;
    }
    /// \brief Задамо ІД джерела
    void setSourceID(const SOURCE_TYPE & source_type)
    {
        if (m_frame)
            m_frame->source_id = source_type;
    }
    /// \brief Тип даних
    /// \return
    inline PAYLOAD_TYPE payloadType() const
    {
        if (m_frame)
            return m_frame->payload_type;

        return PAYLOAD_TYPE::PAYLOAD_TYPE_UNSUPPORTED;
    }
    /// \brief Запишемо тип даних відліків сигналу
    void setPayloadType(const PAYLOAD_TYPE & payload_type)
    {
        if (m_frame)
            m_frame->payload_type = payload_type;
    }
    /// \brief Розмір даних без заголовку
    /// \return
    inline std::uint32_t payloadSize() const
    {
        if (m_frame)
            return m_frame->payload_size;

        return -1;
    }

    /// \brief Задамо розмір даних
    void setPayloadSize(const std::int32_t & size)
    {
        if (m_frame)
            m_frame->payload_size = size;
    }

    /// \brief Вказівник на дані
    /// \return
    char * payload() { return m_payload; }

    char * payload() const { return m_payload; }

    /// \brief Розмір типу даних
    /// \return
    inline std::uint8_t typeSize() { return m_type_size; }

    inline int size() const { return m_frame_size; };
    inline char * data() { return buffer.data(); }

    /// \brief К-сть відліків сигналу
    /// \return
    std::uint32_t totalElements() const { return m_elements_num; };

protected:
    std::vector<char> buffer;         // весь масив даних
    std::uint32_t m_frame_size   = 0; // розмір всього блоку даних
    std::uint32_t m_elements_num = 0; // к-сть відліків сигналу
    std::uint8_t m_type_size     = 0; // sizeof(uint8_t), sizeof(uint16_t) ...
    struct frame * m_frame;           // вказівник на заголовок
    char * m_payload;                 // вказівник на дані оцифрованих відліків
};

template<typename T>
class DataSourceBuffer : public DataSourceBufferInterface
{
public:
    DataSourceBuffer(const std::int32_t & frame_size):
        DataSourceBufferInterface()
    {
        m_frame_size   = frame_size;
        m_type_size    = sizeof(T);
        m_elements_num = (m_frame_size - FRAME_HEADER_SIZE) / m_type_size;
        buffer.resize(m_frame_size);

        m_frame   = reinterpret_cast<struct frame *>(buffer.data());
        m_frame->payload_size = (m_frame_size - FRAME_HEADER_SIZE);
        m_payload = reinterpret_cast<char *>(buffer.data() + FRAME_HEADER_SIZE);
    }

    virtual ~DataSourceBuffer() {}
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEBUFFER_H
