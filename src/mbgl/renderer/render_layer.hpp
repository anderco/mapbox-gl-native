#pragma once
#include <mbgl/layout/layout.hpp>
#include <mbgl/renderer/render_pass.hpp>
#include <mbgl/style/layer_impl.hpp>
#include <mbgl/style/layer_type.hpp>
#include <mbgl/tile/geometry_tile_data.hpp>
#include <mbgl/util/mat4.hpp>

#include <memory>
#include <string>

namespace mbgl {

class Bucket;
class BucketParameters;
class TransitionParameters;
class PropertyEvaluationParameters;
class PaintParameters;
class RenderSource;
class RenderTile;
class TransformState;

class RenderLayer {
protected:
    RenderLayer(style::LayerType, Immutable<style::Layer::Impl>);

    const style::LayerType type;

public:
    static std::unique_ptr<RenderLayer> create(Immutable<style::Layer::Impl>);

    virtual ~RenderLayer() = default;

    // Begin transitions for any properties that have changed since the last frame.
    virtual void transition(const TransitionParameters&) = 0;

    // Fully evaluate possibly-transitioning paint properties based on a zoom level.
    virtual void evaluate(const PropertyEvaluationParameters&) = 0;

    // Returns true if any paint properties have active transitions.
    virtual bool hasTransition() const = 0;

    // Returns true if the layer has a pattern property and is actively crossfading.
    virtual bool hasCrossfade() const = 0;

    // Returns true if the layer requires 3D rendering pass.
    virtual bool has3D() const;

    // Returns true if the layer has symbols.
    virtual bool hasSymbols() const;

    // Check whether this layer is of the given subtype.
    template <class T>
    bool is() const;

    // Dynamically cast this layer to the given subtype.
    template <class T>
    T* as() {
        return is<T>() ? reinterpret_cast<T*>(this) : nullptr;
    }

    template <class T>
    const T* as() const {
        return is<T>() ? reinterpret_cast<const T*>(this) : nullptr;
    }

    const std::string& getID() const;

    // Checks whether this layer needs to be rendered in the given render pass.
    bool hasRenderPass(RenderPass) const;

    // Checks whether this layer can be rendered.
    bool needsRendering(float zoom) const;

    virtual void render(PaintParameters&, RenderSource*) = 0;

    // Check wether the given geometry intersects
    // with the feature
    virtual bool queryIntersectsFeature(
            const GeometryCoordinates&,
            const GeometryTileFeature&,
            const float,
            const TransformState&,
            const float,
            const mat4&) const { return false; };

    virtual std::unique_ptr<Bucket> createBucket(const BucketParameters&, const std::vector<const RenderLayer*>&) const = 0;
    virtual std::unique_ptr<Layout> createLayout(const BucketParameters&,
                                               const std::vector<const RenderLayer*>&,
                                               std::unique_ptr<GeometryTileLayer>,
                                               GlyphDependencies&,
                                               ImageDependencies&) const {
        return nullptr;
    }

    using RenderTiles = std::vector<std::reference_wrapper<RenderTile>>;
    void setRenderTiles(RenderTiles, const TransformState&);

    // Private implementation
    Immutable<style::Layer::Impl> baseImpl;
    void setImpl(Immutable<style::Layer::Impl>);

    virtual void markContextDestroyed();

    // TODO: Figure out how to remove this or whether layers other than
    // RenderHeatmapLayer and RenderLineLayer need paint property updates,
    // similar to color ramp. Temporarily moved to the base.
    virtual void update();

    // TODO: Only for background layers.
    virtual optional<Color> getSolidBackground() const;

    const RenderTiles& getRenderTiles() const { return renderTiles; }

    friend std::string layoutKey(const RenderLayer&);

protected:
    // Checks whether the current hardware can render this layer. If it can't, we'll show a warning
    // in the console to inform the developer.
    void checkRenderability(const PaintParameters&, uint32_t activeBindingCount);

    // Code specific to RenderTiles sorting / filtering
    virtual RenderTiles filterRenderTiles(RenderTiles) const;
    virtual void sortRenderTiles(const TransformState&);
    using FilterFunctionPtr = bool (*)(RenderTile&);
    RenderTiles filterRenderTiles(RenderTiles, FilterFunctionPtr) const;
    virtual bool needsClipping() const;

protected:
    // Stores current set of tiles to be rendered for this layer.
    std::vector<std::reference_wrapper<RenderTile>> renderTiles;

    // Stores what render passes this layer is currently enabled for. This depends on the
    // evaluated StyleProperties object and is updated accordingly.
    RenderPass passes = RenderPass::None;

private:
    // Some layers may not render correctly on some hardware when the vertex attribute limit of
    // that GPU is exceeded. More attributes are used when adding many data driven paint properties
    // to a layer.
    bool hasRenderFailures = false;
};

} // namespace mbgl
