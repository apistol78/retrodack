#include <Core/Thread/Acquire.h>
#include <Ui/Canvas.h>

#include "Emulator/SignalView.h"

using namespace traktor;

bool SignalView::create(ui::Widget* parent, uint32_t mode)
{
	if (!ui::Widget::create(parent))
		return false;

	addEventHandler< ui::PaintEvent >(this, &SignalView::eventPaint);

	m_deltas.push_back({ 0.0f, 0 });
	m_current = 0;
	m_mode = mode;
	return true;
}

void SignalView::set(int32_t id, uint32_t value)
{
	if (value != m_current)
	{
		T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
		const float T = m_timer.getElapsedTime();
		m_deltas.push_back({ T, value });
		m_current = value;
	}
}

void SignalView::eventPaint(ui::PaintEvent* event)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
	ui::Canvas& canvas = event->getCanvas();

	ui::Rect rcUpdate = event->getUpdateRect();
	ui::Rect rc = getInnerRect();

	canvas.setBackground(Color4ub(40, 40, 40, 255));
	canvas.setForeground(Color4ub(255, 255, 255, 200));

	canvas.fillRect(rcUpdate);

	const float T = m_timer.getElapsedTime();
	int32_t x = rc.right;
	uint32_t s = m_current;

	if (m_mode == 0)	// Digital
	{
		for (int32_t i = m_deltas.size() - 1; i >= 0; --i)
		{
			int32_t y0 = (s != 0) ? rc.top + pixel(4_ut) : rc.bottom - pixel(4_ut);
			int32_t y1 = (s == 0) ? rc.top + pixel(4_ut) : rc.bottom - pixel(4_ut);
			int32_t dx = rc.right - ((T - m_deltas[i].T) / m_duration) * rc.getSize().cx;

			canvas.drawLine(x, y0, dx, y0);
			canvas.drawLine(dx, y0, dx, y1);

			s = s ? 0 : 1;
			x = dx;
			if (x < 0)
				break;
		}
	}
	else if (m_mode == 1)	// Numbers
	{
		for (int32_t i = m_deltas.size() - 1; i >= 0; --i)
		{
			int32_t y0 = rc.top + pixel(4_ut);
			int32_t y1 = rc.bottom - pixel(4_ut);
			int32_t dx = rc.right - ((T - m_deltas[i].T) / m_duration) * rc.getSize().cx;

			int32_t mx = std::min(pixel(2_ut), std::abs(dx - x) / 2);

			canvas.drawLine(x, (y0 + y1) / 2, x - mx, y0);
			canvas.drawLine(x, (y0 + y1) / 2, x - mx, y1);

			canvas.drawLine(dx, (y0 + y1) / 2, dx + mx, y0);
			canvas.drawLine(dx, (y0 + y1) / 2, dx + mx, y1);

			canvas.drawLine(x - mx, y0, dx + mx, y0);
			canvas.drawLine(x - mx, y1, dx + mx, y1);

			x = dx;
			if (x < 0)
				break;
		}
	}

	event->consume();
}
