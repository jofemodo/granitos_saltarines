# -*- coding: utf-8 -*-
# ******************************************************************************
# ZYNTHIAN PROJECT: Zynthian widget for "Granitos saltarines"
#
# Plugin: <https://zynthian.org/plugins/granitos_saltarines>
# Declared as a zyngui:widget LV2 UI (see the bundle's manifest.ttl). Half height.
#
# Shows the delay buffer as a strip, with:
#   - a line where the grains read by default (write head minus the delay time),
#   - one dot per sounding grain, at the position it is really reading (up to 16 grains). The dot
#     swells and shrinks with the grain's own level (its Hann window): newborn and dying grains are small,
#     grains in the middle of their life are big,
#   - a ball that bounces above the strip, and whose size follows the Hop control: the bigger the ball,
#     the harder it pulls the grains,
#   - a bar of segments under the strip with how many grains sound at once.
#
# Data: LV2 output control ports ("monitors"), polled ~30 times/s while shown:
#   self.monitors['grain_count'], ['read_pos'] (0..1), ['hop_height'] (0..1),
#   ['grain_pos_1'] .. ['grain_pos_16'] (0..1, or -1 when that grain slot is idle),
#   ['grain_env_1'] .. ['grain_env_16'] (0..1, level of that grain)
# and the Hop control: self.processor.controllers_dict['hop'].
# The widget only draws: it has no controls of its own.
#
# Copyright (C) 2026 Jofemodo
# License: GPL-3.0-or-later
# ******************************************************************************

import tkinter

from zyngui import zynthian_gui_config
from zyngui import zynthian_widget_base

GRAIN_SLOTS = 16                  # grain_pos_N / grain_env_N monitors, must match kGrainPosCount in the DSP
POS_KEYS = [f"grain_pos_{i + 1}" for i in range(GRAIN_SLOTS)]
ENV_KEYS = [f"grain_env_{i + 1}" for i in range(GRAIN_SLOTS)]
SEGMENTS = 16                     # density bar: each segment lights up for 2 grains
GRAINS_PER_SEGMENT = 2            # full bar = 32 grains: more than density x grain size can reach (~28)
LANES = 4                         # dots are spread over this many lanes of the strip, by grain slot
OFFSCREEN = -10


class zynthian_widget_granitos_saltarines(zynthian_widget_base.zynthian_widget_base):

    def __init__(self, parent):
        super().__init__(parent)
        # Half height: the list of pages stays visible above the widget
        self.rows = max(1, self.rows // 2)

        self.widget_canvas = tkinter.Canvas(self, highlightthickness=0, relief='flat',
                                            bg=zynthian_gui_config.color_bg)
        self.widget_canvas.grid(sticky='news')

        # All canvas items are created once here; refresh_gui() only moves or recolours them
        c = self.widget_canvas
        self.strip = c.create_rectangle(0, 0, 0, 0, fill=zynthian_gui_config.color_panel_bg, width=0)
        self.marker = c.create_line(0, 0, 0, 0, fill=zynthian_gui_config.color_hl, width=2)
        self.dots = [c.create_oval(OFFSCREEN, OFFSCREEN, OFFSCREEN, OFFSCREEN,
                                   fill=zynthian_gui_config.color_hl, width=0)
                     for _ in range(GRAIN_SLOTS)]
        self.ball = c.create_oval(0, 0, 0, 0, fill=zynthian_gui_config.color_on, width=0)
        self.segments = [c.create_rectangle(0, 0, 0, 0, fill=zynthian_gui_config.color_panel_bg, width=0)
                         for _ in range(SEGMENTS)]

        self.last_state = None  # what was last drawn: skip the redraw when nothing changed

    def on_size(self, event):
        if not super().on_size(event):
            return
        w, h = self.width, self.height
        self.x0 = int(w * 0.02)  # timeline: 2 % side margins
        self.x1 = int(w * 0.98)
        self.strip_top = int(h * 0.60)
        self.strip_bot = int(h * 0.86)
        self.max_hop = h * 0.45          # how high the ball can bounce above the strip
        # The ball is as big as the Hop control says: small when the grains hardly follow it, big when they do
        self.ball_r_min = max(2, int(h * 0.025))
        self.ball_r_max = max(4, int(h * 0.075))
        # A dot is as big as its grain's level: as big as its lane at the top of the grain's window, a pin at the ends
        self.lane_h = (self.strip_bot - self.strip_top) / LANES
        self.dot_r_min = 1
        self.dot_r_max = max(2, int(self.lane_h * 0.5))

        # Density bar under the strip: SEGMENTS cells across the strip's width
        seg_w = (self.x1 - self.x0) / SEGMENTS
        seg_y0 = int(h * 0.90)
        seg_y1 = int(h * 0.98)
        c = self.widget_canvas
        c.coords(self.strip, self.x0, self.strip_top, self.x1, self.strip_bot)
        for i, seg in enumerate(self.segments):
            sx0 = self.x0 + i * seg_w + 2
            sx1 = self.x0 + (i + 1) * seg_w - 2
            c.coords(seg, sx0, seg_y0, sx1, seg_y1)
        self.last_state = None           # redraw everything with the new geometry

    def get_hop(self):
        """Hop control as a fraction (0..1). 1 if it can't be read: a full-size ball is the safe default."""
        try:
            zctrl = self.processor.controllers_dict['hop']
            lo, hi = zctrl.value_min, zctrl.value_max
            return min(1.0, max(0.0, (zctrl.value - lo) / (hi - lo)))
        except Exception:
            return 1.0

    def refresh_gui(self):
        # Called every ~32 ms while shown: only touch the canvas when something moved
        if not hasattr(self, 'strip_top'):
            return  # on_size not called yet
        m = self.monitors or {}
        count = m.get('grain_count')
        pos = m.get('read_pos')
        hop = m.get('hop_height')
        if count is None or pos is None or hop is None:
            return

        span = self.x1 - self.x0
        n = max(0, int(round(count)))
        px = int(self.x0 + min(1.0, max(0.0, pos)) * span)
        hpx = int(min(1.0, max(0.0, hop)) * self.max_hop)
        ball_r = int(round(self.ball_r_min + (self.ball_r_max - self.ball_r_min) * self.get_hop()))
        # Each grain: (x pixel, radius in pixels), or None when its slot is idle
        dots = []
        for kp, ke in zip(POS_KEYS, ENV_KEYS):
            v = m.get(kp)
            if v is None or v < 0.0:
                dots.append(None)
                continue
            env = min(1.0, max(0.0, m.get(ke) or 0.0))
            dots.append((int(self.x0 + min(1.0, v) * span),
                         int(round(self.dot_r_min + (self.dot_r_max - self.dot_r_min) * env))))
        dots = tuple(dots)
        state = (n, px, hpx, ball_r, dots)
        if state == self.last_state:
            return
        last = self.last_state
        self.last_state = state

        c = self.widget_canvas
        if last is None or px != last[1]:
            c.coords(self.marker, px, self.strip_top, px, self.strip_bot)
        if last is None or px != last[1] or hpx != last[2] or ball_r != last[3]:
            ball_y = self.strip_top - ball_r - 2 - hpx
            c.coords(self.ball, px - ball_r, ball_y - ball_r, px + ball_r, ball_y + ball_r)

        # One dot per sounding grain, at the position it is reading. The lane (vertical position) is
        # fixed by the grain's slot, so a grain keeps its lane for as long as it sounds.
        if last is None or dots != last[4]:
            for i, dot in enumerate(self.dots):
                d = dots[i]
                if d is None:
                    if last is None or last[4][i] is not None:
                        c.coords(dot, OFFSCREEN, OFFSCREEN, OFFSCREEN, OFFSCREEN)
                else:
                    x, r = d
                    cy = self.strip_top + (i % LANES + 0.5) * self.lane_h
                    c.coords(dot, x - r, cy - r, x + r, cy + r)

        # Density bar: segment i is lit when there are more than 2*i grains
        if last is None or n != last[0]:
            for i, seg in enumerate(self.segments):
                lit = n > i * GRAINS_PER_SEGMENT
                c.itemconfigure(seg, fill=zynthian_gui_config.color_hl if lit else zynthian_gui_config.color_panel_bg)

    # Optional hooks (return True to consume the event, otherwise the control view handles it):
    # def zynpot_cb(self, i, dval): ...
    # def switch(self, i, t): ...
