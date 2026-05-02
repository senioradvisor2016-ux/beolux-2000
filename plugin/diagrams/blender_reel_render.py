"""
Blender headless renderer for BC2000DL photoreal reels.

Two reels:
  - LEFT  : full reel with brown ferric-oxide tape wound around hub.
            Three-spoke acrylic disc (semi-transparent) over wound tape.
  - RIGHT : empty take-up reel — clear acrylic, three-spoke,
            visible center hub with EM TAPE-style label (red dot + yellow ring).

Output: 1024x1024 PNG, transparent background.
Same upper-left key light as plugin convention.

Run: blender -b -P blender_reel_render.py
"""
import bpy, math, os

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       '..', 'juce', 'Source', 'assets')
RES = 1024


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    s = bpy.context.scene
    s.render.engine = 'CYCLES'
    s.cycles.device = 'CPU'
    s.cycles.samples = 64
    s.cycles.use_denoising = True
    s.render.resolution_x = RES
    s.render.resolution_y = RES
    s.render.resolution_percentage = 100
    s.render.image_settings.file_format = 'PNG'
    s.render.image_settings.color_mode = 'RGBA'
    s.render.film_transparent = True
    s.view_settings.view_transform = 'Standard'
    return s


def add_camera():
    bpy.ops.object.camera_add(location=(0, 0, 5))
    cam = bpy.context.object
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = 2.2
    return cam


def add_lighting():
    # Key from upper-left @ 45°
    bpy.ops.object.light_add(type='AREA', location=(-3, -3, 4))
    L1 = bpy.context.object
    L1.data.energy = 200
    L1.data.size = 3.0
    L1.data.color = (1.0, 0.97, 0.92)
    L1.rotation_euler = (math.radians(45), math.radians(-25), math.radians(45))
    # Fill cooler from lower-right
    bpy.ops.object.light_add(type='AREA', location=(2.5, 2.5, 2))
    L2 = bpy.context.object
    L2.data.energy = 60
    L2.data.size = 4.0
    L2.data.color = (0.85, 0.92, 1.0)
    L2.rotation_euler = (math.radians(60), math.radians(20), math.radians(-130))


def make_brown_oxide_material():
    m = bpy.data.materials.new('FerricOxide')
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new('ShaderNodeOutputMaterial'); out.location = (700, 0)
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled'); bsdf.location = (400, 0)
    bsdf.inputs['Base Color'].default_value = (0.30, 0.16, 0.08, 1.0)  # ferric brown
    bsdf.inputs['Roughness'].default_value = 0.55
    bsdf.inputs['Metallic'].default_value = 0.05
    # Concentric ring pattern for tape windings
    tex = nt.nodes.new('ShaderNodeTexCoord'); tex.location = (-400, 0)
    sep = nt.nodes.new('ShaderNodeVectorMath'); sep.location = (-200, 0)
    sep.operation = 'LENGTH'
    nt.links.new(tex.outputs['Generated'], sep.inputs[0])
    wave = nt.nodes.new('ShaderNodeTexWave'); wave.location = (50, 100)
    wave.wave_type = 'RINGS'
    wave.inputs['Scale'].default_value = 18.0
    wave.inputs['Distortion'].default_value = 0.5
    wave.inputs['Detail'].default_value = 1.0
    ramp = nt.nodes.new('ShaderNodeValToRGB'); ramp.location = (250, 100)
    ramp.color_ramp.elements[0].color = (0.18, 0.10, 0.04, 1)
    ramp.color_ramp.elements[1].color = (0.40, 0.22, 0.10, 1)
    nt.links.new(wave.outputs['Color'], ramp.inputs['Fac'])
    nt.links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    return m


def make_acrylic_material():
    m = bpy.data.materials.new('Acrylic')
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value = (0.95, 0.94, 0.88, 1.0)
    p.inputs['Transmission Weight'].default_value = 0.85
    p.inputs['Roughness'].default_value = 0.06
    p.inputs['IOR'].default_value = 1.49
    return m


def make_metal_material(rgb=(0.78, 0.76, 0.72), rough=0.3):
    m = bpy.data.materials.new('Metal')
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value = (*rgb, 1.0)
    p.inputs['Metallic'].default_value = 1.0
    p.inputs['Roughness'].default_value = rough
    return m


def make_red_label():
    m = bpy.data.materials.new('RedLabel')
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value = (0.78, 0.10, 0.10, 1.0)
    p.inputs['Roughness'].default_value = 0.65
    return m


def make_yellow_label():
    m = bpy.data.materials.new('YellowLabel')
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value = (0.92, 0.82, 0.18, 1.0)
    p.inputs['Roughness'].default_value = 0.55
    return m


def build_reel(full_tape=True):
    """Build a reel scene. If full_tape, paint brown oxide on inner disc."""
    # Inner tape disc (covers most of the reel's interior)
    if full_tape:
        bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=0.85, depth=0.10,
                                             location=(0, 0, -0.02))
        tape = bpy.context.object
        tape.name = 'Tape'
        tape.data.materials.append(make_brown_oxide_material())
    else:
        # Empty reel — just hub label
        bpy.ops.mesh.primitive_cylinder_add(vertices=64, radius=0.18, depth=0.04,
                                             location=(0, 0, 0))
        hub_yellow = bpy.context.object
        hub_yellow.data.materials.append(make_yellow_label())
        bpy.ops.mesh.primitive_cylinder_add(vertices=64, radius=0.07, depth=0.05,
                                             location=(0, 0, 0.005))
        hub_red = bpy.context.object
        hub_red.data.materials.append(make_red_label())

    # Acrylic 3-spoke disc on top
    bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=1.0, depth=0.06,
                                         location=(0, 0, 0.06))
    disc = bpy.context.object
    disc.name = 'AcrylicDisc'
    disc.data.materials.append(make_acrylic_material())

    # Cut three triangular holes into the disc
    for k in range(3):
        ang = math.radians(k * 120 + 90)
        bpy.ops.mesh.primitive_cube_add(size=1.2,
            location=(math.cos(ang) * 0.45, math.sin(ang) * 0.45, 0.06))
        cutter = bpy.context.object
        cutter.scale = (0.32, 0.32, 0.5)
        cutter.rotation_euler = (0, 0, ang)
        bm = disc.modifiers.new(f'Cut{k}', 'BOOLEAN')
        bm.object = cutter
        bm.operation = 'DIFFERENCE'
        cutter.hide_render = True

    # Center metal hub (spindle)
    bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=0.05, depth=0.20,
                                         location=(0, 0, 0.06))
    spindle = bpy.context.object
    spindle.data.materials.append(make_metal_material(rgb=(0.6, 0.58, 0.55)))


def render_reel(full, out_name):
    s = reset_scene()
    add_camera()
    add_lighting()

    # World transparent
    s.world = bpy.data.worlds.new('Empty')
    s.world.use_nodes = True
    bg = s.world.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = (0, 0, 0, 0)
    bg.inputs['Strength'].default_value = 0.0
    bpy.context.scene.camera = bpy.data.objects['Camera']

    build_reel(full_tape=full)

    fp = os.path.join(OUT_DIR, out_name)
    s.render.filepath = fp
    bpy.ops.render.render(write_still=True)
    print(f"Wrote: {fp}")


print("=== Rendering left reel (full tape) ===")
render_reel(full=True, out_name='reel_left.png')

print("\n=== Rendering right reel (empty) ===")
render_reel(full=False, out_name='reel_right.png')

print("\nDone.")
