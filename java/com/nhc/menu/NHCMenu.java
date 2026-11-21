package com.nhc.menu;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * NHC touch menu. Loaded by the native plugin through InMemoryDexClassLoader.
 * The overlay never consumes game touches itself: only the 18dp top strip (swipe down),
 * the floating button and the open panel are touch targets.
 */
public final class NHCMenu {
    // Feature ids: keep in sync with enum Feature in include/nhc/core.h
    static final int F_MONEY = 0;
    static final int F_GOD = 1;
    static final int F_JETPACK = 2;
    static final int F_VEHICLE = 3;
    static final int F_WEAPON = 4;

    // Implemented in C++ (src/menu.cpp), registered with RegisterNatives
    static native String nAbout();
    static native boolean nAvailable(int feature);
    static native String nReason(int feature);
    static native boolean nEnabled(int feature);
    static native void nSetEnabled(int feature, boolean on);
    static native void nAddMoneyOnce();
    static native void nActivateJetpack();
    static native void nSpawnVehicle(int model);
    static native void nGiveWeapon(int weapon, int ammo);
    static native String[] nVehicles();
    static native String[] nWeapons();
    static native int nGetSetting(String key, int def);
    static native void nSetSetting(String key, int value);
    static native String nLastResult();

    private static final int MATCH = ViewGroup.LayoutParams.MATCH_PARENT;
    private static final int WRAP = ViewGroup.LayoutParams.WRAP_CONTENT;

    private static final int[] BG = {0xFF0A0A0A, 0xFF000000, 0xFF141414};
    private static final int[] CARD = {0xFF161616, 0xFF0E0E0E, 0xFF1F1F1F};
    private static final int[] ACCENT = {0xFFE11D2E, 0xFFB3001B, 0xFFFF4D4D};
    private static final String[] THEME_NAMES = {"Classic Red", "Crimson", "Graphite Red"};
    private static final String[] VIS_NAMES = {"Swipe + button", "Button only", "Swipe only"};
    private static final int TEXT = 0xFFF2F2F2;
    private static final int DIM = 0xFF9A9A9A;
    private static final int[] AMMO_STEPS = {1, 5, 10, 25, 50, 100, 250, 500, 1000, 5000, 9999};

    private static Activity activity;
    private static FrameLayout overlay;
    private static View scrim;
    private static View strip;
    private static TextView floatBtn;
    private static LinearLayout panel;
    private static LinearLayout body;
    private static TextView footer;
    private static TextView backBtn;
    private static TextView pageTitle;

    private static boolean open;
    private static String page = "main";
    private static int scale = 100;
    private static int theme;
    private static int visibility;
    private static int selVehicle;
    private static int selWeapon;
    private static int ammo = 100;
    private static String[] vehicles = new String[0];
    private static String[] weapons = new String[0];
    private static float stripDownY;

    private NHCMenu() {}

    /** Called once on the Android UI thread by the native plugin. */
    public static void start(Activity a) {
        if (overlay != null) return;
        activity = a;
        scale = nGetSetting("UiScale", 100);
        theme = nGetSetting("Theme", 0);
        visibility = nGetSetting("Visibility", 0);
        vehicles = nVehicles();
        weapons = nWeapons();
        if (weapons.length > 0) ammo = weaponDefaultAmmo(0);
        buildOverlay();
    }

    // ------------------------------------------------------------------ actions
    // Plain classes instead of lambdas on purpose: the CI compiles against the SDK android.jar,
    // which does not always contain LambdaMetafactory, so a lambda breaks the DEX step.
    private static final int A_GO = 0;
    private static final int A_CLOSE = 1;
    private static final int A_BACK = 2;
    private static final int A_MONEY = 3;
    private static final int A_JETPACK = 4;
    private static final int A_SPAWN = 5;
    private static final int A_GIVE = 6;
    private static final int A_PICK_VEHICLE = 7;
    private static final int A_PICK_WEAPON = 8;
    private static final int A_AMMO_DOWN = 9;
    private static final int A_AMMO_UP = 10;
    private static final int A_CYCLE_VIS = 11;
    private static final int A_SCALE_DOWN = 12;
    private static final int A_SCALE_UP = 13;
    private static final int A_CYCLE_THEME = 14;
    private static final int A_TOGGLE = 15;
    private static final int A_EXPLAIN = 16;
    private static final int A_FOOTER_NOW = 17;
    private static final int A_PANEL_HIDDEN = 18;

    private static final class Act implements View.OnClickListener, Runnable {
        private final int kind;
        private final String target;
        private final int n;

        Act(int kind, String target, int n) {
            this.kind = kind;
            this.target = target;
            this.n = n;
        }

        @Override
        public void onClick(View v) {
            perform(kind, target, n);
        }

        @Override
        public void run() {
            perform(kind, target, n);
        }
    }

    private static Act act(int kind) {
        return new Act(kind, null, 0);
    }

    private static Act act(int kind, String target) {
        return new Act(kind, target, 0);
    }

    private static Act act(int kind, int n) {
        return new Act(kind, null, n);
    }

    private static void perform(int kind, String target, int n) {
        switch (kind) {
            case A_GO:
                go(target);
                break;
            case A_CLOSE:
                closePanel();
                break;
            case A_BACK:
                goBack();
                break;
            case A_MONEY:
                nAddMoneyOnce();
                refreshFooter();
                break;
            case A_JETPACK:
                nActivateJetpack();
                refreshFooter();
                break;
            case A_SPAWN:
                if (vehicles.length > 0) nSpawnVehicle(toInt(field(vehicles[selVehicle], 0), 0));
                refreshFooter();
                break;
            case A_GIVE:
                if (weapons.length > 0) nGiveWeapon(toInt(field(weapons[selWeapon], 0), 0), ammo);
                refreshFooter();
                break;
            case A_PICK_VEHICLE:
                selVehicle = n;
                go("vehicles");
                break;
            case A_PICK_WEAPON:
                selWeapon = n;
                ammo = weaponDefaultAmmo(n);
                go("weapons");
                break;
            case A_AMMO_DOWN:
                ammo = stepAmmo(-1);
                showPage();
                break;
            case A_AMMO_UP:
                ammo = stepAmmo(1);
                showPage();
                break;
            case A_CYCLE_VIS:
                visibility = (visibility + 1) % VIS_NAMES.length;
                nSetSetting("Visibility", visibility);
                applyVisibility();
                showPage();
                break;
            case A_SCALE_DOWN:
                changeScale(-10);
                break;
            case A_SCALE_UP:
                changeScale(10);
                break;
            case A_CYCLE_THEME:
                theme = (theme + 1) % THEME_NAMES.length;
                nSetSetting("Theme", theme);
                rebuild("settings");
                break;
            case A_TOGGLE:
                if (nAvailable(n)) nSetEnabled(n, !nEnabled(n));
                else nSetEnabled(n, true);   // records the NOT IMPLEMENTED reason in the footer
                showPage();
                break;
            case A_EXPLAIN:
                footer.setText("NOT IMPLEMENTED: " + nReason(n));
                break;
            case A_FOOTER_NOW:
                footer.setText(nLastResult());
                break;
            case A_PANEL_HIDDEN:
                if (!open) {
                    panel.setVisibility(View.GONE);
                    scrim.setVisibility(View.GONE);
                }
                break;
            default:
                break;
        }
    }

    // ------------------------------------------------------------------ helpers

    private static int dp(float v) {
        float d = activity.getResources().getDisplayMetrics().density;
        return Math.round(v * d * scale / 100f);
    }

    private static float sp(float v) {
        return v * scale / 100f;
    }

    private static GradientDrawable round(int fill, int radiusPx, int strokePx, int strokeColor) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(fill);
        g.setCornerRadius(radiusPx);
        if (strokePx > 0) g.setStroke(strokePx, strokeColor);
        return g;
    }

    private static TextView text(String s, float size, int color, boolean bold) {
        TextView t = new TextView(activity);
        t.setText(s);
        t.setTextColor(color);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp(size));
        if (bold) t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    private static String field(String entry, int index) {
        String[] parts = entry.split("\\|");
        return index < parts.length ? parts[index] : "";
    }

    private static int toInt(String s, int def) {
        try {
            return Integer.parseInt(s);
        } catch (NumberFormatException e) {
            return def;
        }
    }

    private static int weaponDefaultAmmo(int index) {
        if (index < 0 || index >= weapons.length) return 100;
        return toInt(field(weapons[index], 2), 100);
    }

    // ------------------------------------------------------------------ overlay

    private static void buildOverlay() {
        overlay = new FrameLayout(activity);

        scrim = new View(activity);
        scrim.setBackgroundColor(0x66000000);
        scrim.setVisibility(View.GONE);
        scrim.setOnClickListener(act(A_CLOSE));
        overlay.addView(scrim, new FrameLayout.LayoutParams(MATCH, MATCH));

        strip = new View(activity);
        FrameLayout.LayoutParams stripLp = new FrameLayout.LayoutParams(MATCH, dp(18));
        stripLp.gravity = Gravity.TOP;
        strip.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent e) {
                int action = e.getActionMasked();
                if (action == MotionEvent.ACTION_DOWN) {
                    stripDownY = e.getRawY();
                } else if (action == MotionEvent.ACTION_MOVE && e.getRawY() - stripDownY > dp(56)) {
                    stripDownY = Float.MAX_VALUE;
                    openPanel();
                }
                return true;
            }
        });
        overlay.addView(strip, stripLp);

        buildFloatingButton();
        buildPanel();

        overlay.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int l, int t, int r, int b, int ol, int ot, int orr, int ob) {
                if (r - l != orr - ol || b - t != ob - ot) onSizeChanged();
            }
        });
        activity.addContentView(overlay, new ViewGroup.LayoutParams(MATCH, MATCH));
        applyVisibility();
    }

    private static void buildFloatingButton() {
        final int size = dp(44);
        floatBtn = text("N", 18, Color.WHITE, true);
        floatBtn.setGravity(Gravity.CENTER);
        floatBtn.setBackground(round(ACCENT[theme], size / 2, dp(2), 0xFF000000));
        DisplayMetrics dm = activity.getResources().getDisplayMetrics();
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(size, size);
        lp.gravity = Gravity.TOP | Gravity.LEFT;
        lp.leftMargin = dm.widthPixels - size - dp(10);
        lp.topMargin = dp(64);
        floatBtn.setOnTouchListener(new View.OnTouchListener() {
            float sx, sy;
            int startLeft, startTop;
            boolean moved;

            @Override
            public boolean onTouch(View v, MotionEvent e) {
                FrameLayout.LayoutParams p = (FrameLayout.LayoutParams) v.getLayoutParams();
                switch (e.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        sx = e.getRawX();
                        sy = e.getRawY();
                        startLeft = p.leftMargin;
                        startTop = p.topMargin;
                        moved = false;
                        return true;
                    case MotionEvent.ACTION_MOVE: {
                        float dx = e.getRawX() - sx;
                        float dy = e.getRawY() - sy;
                        if (Math.abs(dx) > dp(6) || Math.abs(dy) > dp(6)) moved = true;
                        if (moved) {
                            p.leftMargin = clampButton(startLeft + Math.round(dx), true);
                            p.topMargin = clampButton(startTop + Math.round(dy), false);
                            v.setLayoutParams(p);
                        }
                        return true;
                    }
                    case MotionEvent.ACTION_UP:
                        if (!moved) {
                            if (open) closePanel();
                            else openPanel();
                        }
                        return true;
                    default:
                        return true;
                }
            }
        });
        overlay.addView(floatBtn, lp);
    }

    private static int clampButton(int value, boolean horizontal) {
        int limit = horizontal ? overlayWidth() : overlayHeight();
        int max = Math.max(0, limit - dp(44));
        return Math.max(0, Math.min(value, max));
    }

    private static int overlayWidth() {
        if (overlay != null && overlay.getWidth() > 0) return overlay.getWidth();
        return activity.getResources().getDisplayMetrics().widthPixels;
    }

    private static int overlayHeight() {
        if (overlay != null && overlay.getHeight() > 0) return overlay.getHeight();
        return activity.getResources().getDisplayMetrics().heightPixels;
    }

    private static void onSizeChanged() {
        FrameLayout.LayoutParams p = (FrameLayout.LayoutParams) floatBtn.getLayoutParams();
        p.leftMargin = clampButton(p.leftMargin, true);
        p.topMargin = clampButton(p.topMargin, false);
        floatBtn.setLayoutParams(p);
        if (open) layoutPanel();
    }

    private static void applyVisibility() {
        floatBtn.setVisibility(visibility == 2 ? View.GONE : View.VISIBLE);
        strip.setVisibility(visibility == 1 ? View.GONE : View.VISIBLE);
    }

    // ------------------------------------------------------------------ panel

    private static void buildPanel() {
        panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackground(round(BG[theme], dp(18), dp(1), ACCENT[theme]));
        panel.setPadding(dp(14), dp(12), dp(14), dp(10));
        panel.setClickable(true);   // swallow touches so they do not reach the game
        panel.setVisibility(View.GONE);

        LinearLayout head = new LinearLayout(activity);
        head.setOrientation(LinearLayout.HORIZONTAL);
        head.setGravity(Gravity.CENTER_VERTICAL);

        LinearLayout titles = new LinearLayout(activity);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.addView(text("NHC", 26, ACCENT[theme], true));
        TextView sub = text("NAPOLEON HORORSS-CENTER", 10, DIM, true);
        sub.setLetterSpacing(0.12f);
        titles.addView(sub);
        head.addView(titles, new LinearLayout.LayoutParams(0, WRAP, 1f));

        TextView close = text("CLOSE", 12, TEXT, true);
        close.setPadding(dp(12), dp(8), dp(12), dp(8));
        close.setBackground(round(CARD[theme], dp(10), 0, 0));
        close.setOnClickListener(act(A_CLOSE));
        head.addView(close);
        panel.addView(head, new LinearLayout.LayoutParams(MATCH, WRAP));

        LinearLayout nav = new LinearLayout(activity);
        nav.setOrientation(LinearLayout.HORIZONTAL);
        nav.setGravity(Gravity.CENTER_VERTICAL);
        nav.setPadding(0, dp(10), 0, dp(6));
        backBtn = text("< BACK", 12, ACCENT[theme], true);
        backBtn.setPadding(0, dp(4), dp(14), dp(4));
        backBtn.setOnClickListener(act(A_BACK));
        nav.addView(backBtn);
        pageTitle = text("NHC MOD MENU", 12, DIM, true);
        nav.addView(pageTitle);
        panel.addView(nav, new LinearLayout.LayoutParams(MATCH, WRAP));

        ScrollView scroll = new ScrollView(activity);
        scroll.setVerticalScrollBarEnabled(false);
        body = new LinearLayout(activity);
        body.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(body, new ViewGroup.LayoutParams(MATCH, WRAP));
        panel.addView(scroll, new LinearLayout.LayoutParams(MATCH, 0, 1f));

        footer = text("", 11, DIM, false);
        footer.setPadding(0, dp(8), 0, 0);
        panel.addView(footer, new LinearLayout.LayoutParams(MATCH, WRAP));

        overlay.addView(panel, new FrameLayout.LayoutParams(dp(320), dp(400)));
    }

    private static void layoutPanel() {
        int w = overlayWidth();
        int h = overlayHeight();
        boolean land = w > h;
        int pw = land ? Math.min(Math.round(w * 0.5f), dp(420)) : Math.min(Math.round(w * 0.94f), dp(440));
        int ph = Math.round(h * (land ? 0.90f : 0.80f));
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(pw, ph);
        lp.gravity = land ? (Gravity.TOP | Gravity.END) : (Gravity.TOP | Gravity.CENTER_HORIZONTAL);
        lp.topMargin = dp(24);
        if (land) lp.rightMargin = dp(12);
        panel.setLayoutParams(lp);
    }

    private static void openPanel() {
        if (open) return;
        open = true;
        page = "main";
        showPage();
        layoutPanel();
        scrim.setVisibility(View.VISIBLE);
        scrim.setAlpha(0f);
        scrim.animate().alpha(1f).setDuration(160).start();
        panel.setVisibility(View.VISIBLE);
        panel.setAlpha(0f);
        panel.setTranslationY(-dp(40));
        panel.animate().alpha(1f).translationY(0f).setDuration(200).start();
    }

    private static void closePanel() {
        if (!open) return;
        open = false;
        scrim.animate().alpha(0f).setDuration(160).start();
        panel.animate().alpha(0f).translationY(-dp(40)).setDuration(160).withEndAction(act(A_PANEL_HIDDEN)).start();
    }

    /** Rebuilds the whole UI after the scale or theme changed. */
    private static void rebuild(String reopenPage) {
        boolean wasOpen = open;
        open = false;
        if (overlay != null && overlay.getParent() instanceof ViewGroup)
            ((ViewGroup) overlay.getParent()).removeView(overlay);
        overlay = null;
        buildOverlay();
        if (wasOpen) {
            openPanel();
            page = reopenPage;
            showPage();
        }
    }

    // ------------------------------------------------------------------ pages

    private static void goBack() {
        switch (page) {
            case "vehicles_list":
                page = "vehicles";
                break;
            case "weapons_list":
                page = "weapons";
                break;
            case "about":
                page = "settings";
                break;
            default:
                page = "main";
        }
        showPage();
    }

    private static void go(String p) {
        page = p;
        showPage();
    }

    private static void showPage() {
        body.removeAllViews();
        backBtn.setVisibility(page.equals("main") ? View.GONE : View.VISIBLE);
        switch (page) {
            case "player":
                pageTitle.setText("PLAYER");
                pagePlayer();
                break;
            case "vehicles":
                pageTitle.setText("VEHICLES");
                pageVehicles();
                break;
            case "vehicles_list":
                pageTitle.setText("VEHICLE LIST");
                pageVehicleList();
                break;
            case "weapons":
                pageTitle.setText("WEAPONS");
                pageWeapons();
                break;
            case "weapons_list":
                pageTitle.setText("WEAPON SELECTOR");
                pageWeaponList();
                break;
            case "settings":
                pageTitle.setText("SETTINGS");
                pageSettings();
                break;
            case "about":
                pageTitle.setText("ABOUT NHC");
                pageAbout();
                break;
            default:
                pageTitle.setText("NHC MOD MENU");
                pageMain();
        }
        footer.setText(nLastResult());
    }

    private static void pageMain() {
        body.addView(row("PLAYER", "Infinite Money, Invincibility, Jetpack", true, act(A_GO, "player")));
        body.addView(row("VEHICLES", "Vehicle Spawn, Vehicle List", true, act(A_GO, "vehicles")));
        body.addView(row("WEAPONS", "Weapon Selector", true, act(A_GO, "weapons")));
        body.addView(row("SETTINGS", "Menu Visibility, UI Scale, Theme, About NHC", true, act(A_GO, "settings")));
    }

    private static void pagePlayer() {
        body.addView(toggleRow("Infinite Money", "Calls the game's own money cheat every 3 s (+$250,000, also refills health and armor)", F_MONEY));
        body.addView(actionRow("Add $250,000 once", F_MONEY, act(A_MONEY)));
        body.addView(toggleRow("Invincibility", "Blocks CPed::InflictDamage for the player (other damage paths are not covered)", F_GOD));
        body.addView(actionRow("Jetpack: Activate", F_JETPACK, act(A_JETPACK)));
        body.addView(unavailableRow("Jetpack: Deactivate", "NOT IMPLEMENTED - no verified game function removes the jetpack"));
    }

    private static void pageVehicles() {
        String name = vehicles.length > 0 ? field(vehicles[selVehicle], 1) : "-";
        body.addView(actionRow("Vehicle Spawn: " + name, F_VEHICLE, act(A_SPAWN)));
        body.addView(row("Vehicle List", vehicles.length + " vehicles", true, act(A_GO, "vehicles_list")));
    }

    private static void pageVehicleList() {
        for (int i = 0; i < vehicles.length; i++) {
            final int index = i;
            String label = field(vehicles[i], 1) + "  (" + field(vehicles[i], 0) + ")";
            body.addView(row(label, i == selVehicle ? "selected" : "", true, act(A_PICK_VEHICLE, index)));
        }
    }

    private static void pageWeapons() {
        String name = weapons.length > 0 ? field(weapons[selWeapon], 1) : "-";
        body.addView(row("Weapon Selector", name, true, act(A_GO, "weapons_list")));

        LinearLayout card = card();
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.addView(smallButton("-", act(A_AMMO_DOWN)));
        TextView label = text("Ammo: " + ammo, 14, TEXT, true);
        label.setGravity(Gravity.CENTER);
        card.addView(label, new LinearLayout.LayoutParams(0, WRAP, 1f));
        card.addView(smallButton("+", act(A_AMMO_UP)));
        body.addView(card);

        body.addView(actionRow("Give " + name + " (" + ammo + ")", F_WEAPON, act(A_GIVE)));
    }

    private static int stepAmmo(int dir) {
        int idx = 0;
        for (int i = 0; i < AMMO_STEPS.length; i++) {
            if (AMMO_STEPS[i] <= ammo) idx = i;
        }
        if (dir > 0 && AMMO_STEPS[idx] == ammo) idx++;
        else if (dir < 0 && AMMO_STEPS[idx] == ammo) idx--;
        idx = Math.max(0, Math.min(AMMO_STEPS.length - 1, idx));
        return AMMO_STEPS[idx];
    }

    private static void pageWeaponList() {
        for (int i = 0; i < weapons.length; i++) {
            final int index = i;
            String label = field(weapons[i], 1) + "  (" + field(weapons[i], 0) + ")";
            body.addView(row(label, i == selWeapon ? "selected" : "", true, act(A_PICK_WEAPON, index)));
        }
    }

    private static void pageSettings() {
        body.addView(row("Menu Visibility", VIS_NAMES[visibility] + " (tap to change)", true, act(A_CYCLE_VIS)));

        LinearLayout card = card();
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.addView(smallButton("-", act(A_SCALE_DOWN)));
        TextView label = text("UI Scale: " + scale + "%", 14, TEXT, true);
        label.setGravity(Gravity.CENTER);
        card.addView(label, new LinearLayout.LayoutParams(0, WRAP, 1f));
        card.addView(smallButton("+", act(A_SCALE_UP)));
        body.addView(card);

        body.addView(row("Theme", THEME_NAMES[theme] + " (tap to change)", true, act(A_CYCLE_THEME)));
        body.addView(row("About NHC", "Version, compatibility report, feature status", true, act(A_GO, "about")));
    }

    private static void changeScale(int delta) {
        int next = Math.max(60, Math.min(160, scale + delta));
        if (next == scale) return;
        scale = next;
        nSetSetting("UiScale", scale);
        rebuild("settings");
    }

    private static void pageAbout() {
        TextView t = text(nAbout(), 12, TEXT, false);
        t.setTypeface(Typeface.MONOSPACE);
        LinearLayout c = card();
        c.addView(t);
        body.addView(c);
    }

    private static void refreshFooter() {
        // The game thread runs the queued action on the next frame; show the queue state now
        // and the final result a moment later.
        footer.setText(nLastResult());
        footer.postDelayed(act(A_FOOTER_NOW), 700);
    }

    // ------------------------------------------------------------------ rows

    private static LinearLayout card() {
        LinearLayout c = new LinearLayout(activity);
        c.setOrientation(LinearLayout.VERTICAL);
        c.setBackground(round(CARD[theme], dp(12), 0, 0));
        c.setPadding(dp(14), dp(12), dp(14), dp(12));
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(MATCH, WRAP);
        lp.bottomMargin = dp(8);
        c.setLayoutParams(lp);
        return c;
    }

    private static View row(String title, String subtitle, boolean enabled, View.OnClickListener click) {
        LinearLayout c = card();
        c.addView(text(title, 15, enabled ? TEXT : DIM, true));
        if (subtitle != null && subtitle.length() > 0) c.addView(text(subtitle, 11, DIM, false));
        if (enabled) c.setOnClickListener(click);
        return c;
    }

    private static View toggleRow(String title, String description, final int feature) {
        final boolean available = nAvailable(feature);
        final boolean on = available && nEnabled(feature);
        LinearLayout c = card();
        LinearLayout line = new LinearLayout(activity);
        line.setOrientation(LinearLayout.HORIZONTAL);
        line.setGravity(Gravity.CENTER_VERTICAL);
        line.addView(text(title, 15, available ? TEXT : DIM, true), new LinearLayout.LayoutParams(0, WRAP, 1f));
        TextView pill = text(on ? "ON" : "OFF", 12, Color.WHITE, true);
        pill.setGravity(Gravity.CENTER);
        pill.setPadding(dp(14), dp(6), dp(14), dp(6));
        pill.setBackground(round(on ? ACCENT[theme] : 0xFF3A3A3A, dp(14), 0, 0));
        line.addView(pill);
        c.addView(line);
        c.addView(text(description, 11, DIM, false));
        if (!available) c.addView(text("NOT IMPLEMENTED - " + nReason(feature), 11, ACCENT[theme], false));
        c.setOnClickListener(act(A_TOGGLE, feature));
        return c;
    }

    private static View actionRow(String title, final int feature, View.OnClickListener click) {
        boolean available = nAvailable(feature);
        LinearLayout c = card();
        c.addView(text(title, 15, available ? TEXT : DIM, true));
        if (available) {
            c.setOnClickListener(click);
        } else {
            c.addView(text("NOT IMPLEMENTED - " + nReason(feature), 11, ACCENT[theme], false));
            c.setOnClickListener(act(A_EXPLAIN, feature));
        }
        return c;
    }

    private static View unavailableRow(String title, String reason) {
        LinearLayout c = card();
        c.addView(text(title, 15, DIM, true));
        c.addView(text(reason, 11, ACCENT[theme], false));
        return c;
    }

    private static TextView smallButton(String label, View.OnClickListener click) {
        TextView b = text(label, 18, Color.WHITE, true);
        b.setGravity(Gravity.CENTER);
        b.setBackground(round(ACCENT[theme], dp(10), 0, 0));
        b.setLayoutParams(new LinearLayout.LayoutParams(dp(48), dp(40)));
        b.setOnClickListener(click);
        return b;
    }
}
