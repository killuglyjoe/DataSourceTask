#include "DataSourceFrameProcessor.h"
#include <iostream>
#include <ostream>

#ifdef WITH_OPEN_GL
#include "GL/gl.h"
#include "GL/glext.h"
#endif

namespace DATA_SOURCE_TASK
{

#ifdef WITH_OPEN_GL
// вершинний шейдер для перетворення цілого в число з плаваючою крапкою
// static const char * convert_2_float = {"#version 330 core"
//                                        "layout(location = 0) in ivec2 inPosition; // Integer attribute"
//                                        "void main()"
//                                        "{"
//                                        "// Convert integer to float"
//                                        "vec2 floatPosition = vec2(inPosition);"
//                                        "// Pass the converted float values to the fragment shader"
//                                        "gl_Position = vec4(floatPosition, 0.0, 1.0);"
//                                        "}"};
#endif

DataSourceFrameProcessor::DataSourceFrameProcessor(const int & frame_size, const int & num_elements):
    m_frame_size {frame_size}, m_packets_loss {0}
{
    m_buffer = std::make_shared<DataSourceBuffer<float>>(num_elements);
}

bool DataSourceFrameProcessor::validateFrame(std::shared_ptr<DataSourceBufferInterface> buffer, const int & updated_size)
{
    std::lock_guard<std::mutex> lock(m_process_mutex);

    // Timer timer;
    // timer.reset();

    frame * frm                = buffer->frame();
    char * payload             = buffer->payload();

    static int cur_frm_counter = -1;

    if (cur_frm_counter == -1)
    {
        cur_frm_counter = frm->frame_counter;
    }
    else
    {
        // лічільник кадрів
        const int delta = frm->frame_counter - cur_frm_counter;

        if (delta> 1)
        {
            m_packets_loss += frm->frame_counter - cur_frm_counter - 1;
        }
    }

    cur_frm_counter = frm->frame_counter;

    // розмір не відповідає необхідному - втрачений кадр?
    if (updated_size != frameSize())
    {
        ++m_packets_loss;
    }

    // оновимо заголовок
    memcpy(m_buffer->frame(), frm, sizeof(struct frame));

    // - реалізувати максимально обчислювально ефективне перетворення усіх даних
    // до єдиного типу 32 bit IEEE 754 float та приведення до діапазону +/-1.0;
    if (frm->payload_type != PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT)
    {
        convertToFLoat(payload);

        return true;
    }

    // перекладемо дані якшо вони вже в форматі float
    memcpy(m_buffer->payload(), payload, sizeof(struct frame));

    return true;
}

void DataSourceFrameProcessor::convertToFLoat(char * payload)
{
    // #ifdef WITH_OPEN_GL
    //     // GPU
    //     glBegin(GL_POINTS);

    //     /// \todo

    //     glEnd();
    // #else

    // CPU
    for (int i = 0; i < m_buffer->payloadSize(); ++i)
    {
        m_buffer->payload()[i] = static_cast<float>(payload[i]);
    }

    // #endif
}

} // namespace DATA_SOURCE_TASK
