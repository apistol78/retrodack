#pragma once

#include <Core/Containers/AlignedVector.h>
#include <Core/Thread/Semaphore.h>
#include <Core/Timer/Timer.h>
#include <Ui/Widget.h>

class SignalView : public traktor::ui::Widget
{
public:
	bool create(traktor::ui::Widget* parent, uint32_t mode);

	void set(int32_t id, uint32_t value);

private:
	struct Value
	{
		float T;
		uint32_t value;
	};

	traktor::Timer m_timer;
	traktor::Semaphore m_lock;
	traktor::AlignedVector< Value > m_deltas;
	float m_duration = 1.0f;
	uint32_t m_current = 0;
	uint32_t m_mode = 0;

	void eventPaint(traktor::ui::PaintEvent* event);
};
