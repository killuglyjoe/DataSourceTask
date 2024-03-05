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
    virtual ~DataSourceBufferInterface() = default;

    inline struct frame * frame() { return m_frame; };

    /// \brief Фугкція повертає заголовок фрейму
    /// \return
    inline std::int32_t header() const
    {
        if (m_frame)
            return m_frame->magic_word;

        return -1;
    }
    /// \brief
    /// \return
    void setHeader(const std::int32_t & header)
    {
        if (m_frame)
            m_frame->magic_word = header;
    }
    /// \brief
    /// \return
    inline std::int16_t frameCounter() const
    {
        if (m_frame)
            return m_frame->frame_counter;

        return -1;
    }
    /// \brief
    /// \return
    void setFrameCounter(const std::int16_t & cntr)
    {
        if (m_frame)
            m_frame->frame_counter = cntr;
    }
    /// \brief
    /// \return
    inline SOURCE_TYPE sourceId() const
    {
        if (m_frame)
            return m_frame->source_id;

        return SOURCE_TYPE::SOURCE_TYPE_OTHER;
    }
    /// \brief
    void setSourceID(const SOURCE_TYPE & source_type)
    {
        if (m_frame)
            m_frame->source_id = source_type;
    }
    /// \brief
    /// \return
    inline PAYLOAD_TYPE payloadType() const
    {
        if (m_frame)
            return m_frame->payload_type;

        return PAYLOAD_TYPE::PAYLOAD_TYPE_UNSUPPORTED;
    }
    /// \brief
    void setPayloadType(const PAYLOAD_TYPE & payload_type)
    {
        if (m_frame)
            m_frame->payload_type = payload_type;
    }
    /// \brief
    /// \return
    inline std::int32_t payloadSize() const
    {
        if (m_frame)
            return m_frame->payload_size;

        return -1;
    }

    /// \brief
    void setPayloadSize(const std::int32_t & size)
    {
        if (m_frame)
            m_frame->payload_size = size;
    }

    /// \brief Вказівник на дані
    /// \return
    char * payload() {return m_payload;}

    char * payload() const {return m_payload;}

    /// \brief Розмір типу даних
    /// \return
    inline std::uint8_t typeSize() { return m_type_size; }

    inline int size() const { return m_size; };
    inline char * data() { return buffer.data(); }

protected:
    std::vector<char> buffer;     // весь масив даних
    int m_size               = 0; // розмір всього блоку даних
    std::uint8_t m_type_size = 0; // sizeof(uint8_t), sizeof(uint16_t) ...
    struct frame * m_frame;       // вказівник на заголовок
    char         *m_payload;      // вказівник на дані оцифрованих відліків
};

template<typename T>
class DataSourceBuffer : public DataSourceBufferInterface
{
public:
    DataSourceBuffer(const int & num_elements):
        DataSourceBufferInterface()
    {
        m_type_size = sizeof(T);
        m_size      = sizeof(struct frame) + m_type_size * num_elements;
        buffer.resize(m_size);

        m_frame   = reinterpret_cast<struct frame *>(buffer.data());
        m_payload = reinterpret_cast<char *>(buffer.data() + sizeof(struct frame));
    }

    virtual ~DataSourceBuffer() {}
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEBUFFER_H
