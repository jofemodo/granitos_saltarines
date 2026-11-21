# -*- coding: utf-8 -*-
# ******************************************************************************
# ZYNTHIAN PROJECT: Zynthian widget for "Granitos saltarines"
#
# Plugin: <https://zynthian.org/plugins/granitos_saltarines>
# Declared as a zyngui:widget LV2 UI (see the bundle's manifest.ttl). Half height.
#
# Shows the delay buffer as a strip, the read position of the grains as a marker, one dot per
# sounding grain, a bar of segments with the grain density, and a ball that bounces above the
# strip with the current hop.
#
# Data: LV2 output control ports ("monitors"), polled ~30 times/s while shown:
#   self.monitors['grain_count'], ['read_pos'] (0..1), ['hop_height'] (0..1)
# The widget only draws: it has no controls of its own.
#
# Copyright (C) 2026 Jofemodo
# License: GPL-3.0-or-later
# ******************************************************************************

import tkinter

from zyngui import zynthian_gui_config
from zyngui import zynthian_widget_base

MAX_GRAINS = 64  # must match kMaxGrains in the DSP
SEGMENTS = 16    # density bar: each segment lights up for 4 grains
GRAINS_PER_SEGMENT = MAX_GRAINS // SEGMENTS
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
        self.ball = c.create_oval(0, 0, 0, 0, fill=zynthian_gui_config.color_on, width=0)
        self.grains = [c.create_oval(OFFSCREEN, OFFSCREEN, OFFSCREEN, OFFSCREEN,
                                     fill=zynthian_gui_config.color_hl, width=0)
                       for _ in range(MAX_GRAINS)]
        self.segments = [c.create_rectangle(0, 0, 0, 0, fill=zynthian_gui_config.color_panel_bg, width=0)
                         for _ in range(SEGMENTS)]

        self.last_state = None  # (grain count, read px, hop px) last drawn

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
        self.grain_r = max(2, int(h * 0.03))
        self.spread = w * 0.12           # horizontal spread of the grain cloud around the read point

        # Density bar at the top: SEGMENTS cells across the width
        seg_w = (self.x1 - self.x0) / SEGMENTS
        seg_y0 = int(h * 0.08)
        seg_y1 = int(h * 0.26)
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

        n = min(MAX_GRAINS, max(0, int(round(count))))
        px = int(self.x0 + min(1.0, max(0.0, pos)) * (self.x1 - self.x0))
        hpx = int(min(1.0, max(0.0, hop)) * self.max_hop)
        state = (n, px, hpx)
        if state == self.last_state:
            return
        self.last_state = state

        c = self.widget_canvas
        r = self.ball_r
        c.coords(self.marker, px, self.strip_top, px, self.strip_bot)
        ball_y = self.strip_top - r - 2 - hpx
        c.coords(self.ball, px - r, ball_y - r, px + r, ball_y + r)

        # One dot per sounding grain, spread over the strip around the read point
        strip_h = self.strip_bot - self.strip_top
        gr = self.grain_r
        for i, dot in enumerate(self.grains):
            if i < n:
                dx = ((i * 0.618034) % 1.0 - 0.5) * self.spread
                dy = ((i * 0.381966) % 1.0) * strip_h
                cx = px + dx
                cy = self.strip_top + dy
                c.coords(dot, cx - gr, cy - gr, cx + gr, cy + gr)
            else:
                c.coords(dot, OFFSCREEN, OFFSCREEN, OFFSCREEN, OFFSCREEN)

        # Density bar: segment i is lit when there are more than 4*i grains
        for i, seg in enumerate(self.segments):
            lit = n > i * GRAINS_PER_SEGMENT
            c.itemconfigure(seg, fill=zynthian_gui_config.color_hl if lit else zynthian_gui_config.color_panel_bg)

    # Optional hooks (return True to consume the event, otherwise the control view handles it):
    # def zynpot_cb(self, i, dval): ...
    # def switch(self, i, t): ...
