import bpy


def setup_cinematic_compositor():
    scene = bpy.context.scene
    scene.use_nodes = True

    tree = scene.node_tree

    if tree is None:
        raise RuntimeError(
            "Blender compositor node tree unavailable."
        )

    for node in list(tree.nodes):
        tree.nodes.remove(node)

    render_layers = tree.nodes.new(
        type="CompositorNodeRLayers"
    )

    glare = tree.nodes.new(
        type="CompositorNodeGlare"
    )

    glare.glare_type = "FOG_GLOW"

    color_balance = tree.nodes.new(
        type="CompositorNodeColorBalance"
    )

    color_balance.correction_method = (
        "LIFT_GAMMA_GAIN"
    )

    color_balance.lift = (
        0.02,
        0.05,
        0.08,
    )

    color_balance.gamma = (
        1.0,
        0.95,
        0.9,
    )

    color_balance.gain = (
        1.2,
        0.85,
        0.4,
    )

    lens = tree.nodes.new(
        type="CompositorNodeLensdist"
    )

    lens.inputs[
        "Dispersion"
    ].default_value = 0.025

    composite = tree.nodes.new(
        type="CompositorNodeComposite"
    )

    tree.links.new(
        render_layers.outputs["Image"],
        glare.inputs["Image"],
    )

    tree.links.new(
        glare.outputs["Image"],
        color_balance.inputs["Image"],
    )

    tree.links.new(
        color_balance.outputs["Image"],
        lens.inputs["Image"],
    )

    tree.links.new(
        lens.outputs["Image"],
        composite.inputs["Image"],
    )

    scene.render.engine = "CYCLES"
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 816
    scene.render.resolution_percentage = 100


setup_cinematic_compositor()
