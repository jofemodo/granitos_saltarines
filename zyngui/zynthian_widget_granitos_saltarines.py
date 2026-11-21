# -*- coding: utf-8 -*-
# ******************************************************************************
# ZYNTHIAN PROJECT: Zynthian widget for "Granitos saltarines"
#
# Plugin: <https://zynthian.org/plugins/granitos_saltarines>
# Declared as a zyngui:widget LV2 UI (see the bundle's manifest.ttl). Half height.
#
# Shows the delay buffer as a strip, with:
#   - a green line: where the grains read by default (write head minus the delay time),
#   - one dot per sounding grain, at the position it is really reading (up to 16 grains),
#   - an orange ball that bounces above the strip with the current hop,
#   - a bar of segments under the strip with how many grains sound at once.
#
# Data: LV2 output control ports ("monitors"), polled ~30 times/s while shown:
#   self.monitors['grain_count'], ['read_pos'] (0..1), ['hop_height'] (0..1),
#   ['grain_pos_1'] .. ['grain_pos_16'] (0..1, or -1 when that grain slot is idle)
# The widget only draws: it has no controls of its own.
#
# Copyright (C) 2026 Jofemodo
# License: GPL-3.0-or-later
# ******************************************************************************

import tkinter

from zyngui import zynthian_gui_config
from zyngui import zynthian_widget_base

GRAIN_SLOTS = 16                  # grain_pos_N monitors, must match kGrainPosCount in the DSP
POS_KEYS = [f"grain_pos_{i + 1}" for i in range(GRAIN_SLOTS)]
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
        self.ball_r = max(3, int(h * 0.06))
        self.dot_r = max(2, int(h * 0.035))

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
        # x pixel of each grain, or None when its slot is idle
        dots = tuple(int(self.x0 + min(1.0, v) * span) if v is not None and v >= 0.0 else None
                     for v in (m.get(k) for k in POS_KEYS))
        state = (n, px, hpx, dots)
        if state == self.last_state:
            return
        last = self.last_state
        self.last_state = state

        c = self.widget_canvas
        r = self.ball_r
        if last is None or px != last[1]:
            c.coords(self.marker, px, self.strip_top, px, self.strip_bot)
        if last is None or px != last[1] or hpx != last[2]:
            ball_y = self.strip_top - r - 2 - hpx
            c.coords(self.ball, px - r, ball_y - r, px + r, ball_y + r)

        # One dot per sounding grain, at the position it is reading. The lane (vertical position) is
        # fixed by the grain's slot, so a grain keeps its lane for as long as it sounds.
        if last is None or dots != last[3]:
            lane_h = (self.strip_bot - self.strip_top) / LANES
            dr = self.dot_r
            for i, dot in enumerate(self.dots):
                x = dots[i]
                if x is None:
                    if last is None or last[3][i] is not None:
                        c.coords(dot, OFFSCREEN, OFFSCREEN, OFFSCREEN, OFFSCREEN)
                else:
                    cy = self.strip_top + (i % LANES + 0.5) * lane_h
                    c.coords(dot, x - dr, cy - dr, x + dr, cy + dr)

        # Density bar: segment i is lit when there are more than 2*i grains
        if last is None or n != last[0]:
            for i, seg in enumerate(self.segments):
                lit = n > i * GRAINS_PER_SEGMENT
                c.itemconfigure(seg, fill=zynthian_gui_config.color_hl if lit else zynthian_gui_config.color_panel_bg)

    # Optional hooks (return True to consume the event, otherwise the control view handles it):
    # def zynpot_cb(self, i, dval): ...
    # def switch(self, i, t): ...
