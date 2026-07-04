# Web assets live in <repo>/web/ (single source of truth) and are baked
# into src/web_assets.c by tools/gen_web_assets.py — re-run it after
# editing web/*. (ESP-IDF's EMBED_TXTFILES is broken under PlatformIO's
# SCons wrapper, so plain generated C sources are used instead.)
idf_component_register(
    SRCS
        "src/web_server.c"
        "src/web_assets.c"
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        "src"
    REQUIRES
        esp_http_server
        net_mgr
    PRIV_REQUIRES
        log
        json
        app_update
        settings_mgr
        dsp_engine
        display_ui
        audio_source
        heap
)
