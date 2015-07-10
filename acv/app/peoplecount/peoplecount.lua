dofile("../../core/acv.lua")

modules = {
    {
        id = "vi", modtype = "video_in",
        options = {
            recover = "yes"
        }
    },
    {
        id = "logic", modtype = "logic",
        options = {
            thresh = 0.235,
            resize = "160x120"
        }
    }
}

wires = {
    { "vi.vout", "logic.vin" },
    { "vi.vout", "vo.vin" },
    { "logic.vizout", "dbg.vin" }
}

batch_add_modules(modules)
batch_configure_modules(modules)
