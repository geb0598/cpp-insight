#pragma once

namespace insight::viewer {

// -------------------------------------------------
// PanelContext 
// -------------------------------------------------
struct PanelContext {
    bool is_connected     = false;
    bool is_recording     = false;
    bool needs_reset      = false;
    size_t timeline_begin = 0;
    size_t timeline_end   = 0;
};

// -------------------------------------------------
// Panel
// -------------------------------------------------
class Panel {
public:
    explicit Panel(PanelContext& ctx) : ctx_(ctx) {}

    virtual ~Panel() = default;

    virtual void Render() = 0;
    virtual void Reset() {}

protected:
    PanelContext& GetContext() { return ctx_; }

private:
    PanelContext& ctx_;
};

}