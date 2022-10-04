import numpy as np
from mathutils import Matrix
import json

RoughnessMode = {'GGX': 'ggx', 'BECKMANN': 'beckmann',
                 'ASHIKHMIN_SHIRLEY': 'blinn', 'MULTI_GGX': 'ggx'}


def export_texture_node(ctx, tex_node):
    params = {
        'type': 'image'
    }
    # get the relative path to the copied texture from the full path to the original texture
    params['filename'] = ctx.export_texture(tex_node.image)
    # TODO: texture transform (mapping node)
    if tex_node.image.colorspace_settings.name in ['Non-Color', 'Raw', 'Linear']:
        # non color data, tell Darts not to apply gamma conversion to it
        params['raw'] = True
    elif tex_node.image.colorspace_settings.name != 'sRGB':
        ctx.report(
            {'WARNING'}, "Darts only supports sRGB textures for color data.")

    return params


def convert_float_texture_node(ctx, socket):
    params = None

    if socket.is_linked:
        node = socket.links[0].from_node

        if node.bl_idname == "ShaderNodeTexImage":
            params = export_texture_node(ctx, node)
        elif node.bl_idname == "ShaderNodeFresnel":
            if node.inputs['IOR'].is_linked:
                ctx.report(
                    {'WARNING'}, "Textured IOR values are not supported in Darts. Using the default value instead.")

            params = {
                'type': 'fresnel',
                'ior': node.inputs['IOR'].default_value
            }
        else:
            raise NotImplementedError(
                f"Node type {node.bl_idname} is not supported. Only texture and fresnel nodes are supported for float inputs in Darts")
    else:
        if socket.name == 'Roughness':  # roughness values in blender are remapped with a square root
            params = pow(socket.default_value, 2)
        else:
            params = socket.default_value

    return params


def convert_color_texture_node(ctx, socket):
    params = None

    if socket.is_linked:
        node = socket.links[0].from_node

        if node.bl_idname == "ShaderNodeTexImage":
            params = export_texture_node(ctx, node)
        elif node.bl_idname == "ShaderNodeNormalMap":
            if node.space != 'TANGENT':
                raise NotImplementedError(
                    f"Darts only supports tangent-space normal maps")
            params = export_texture_node(ctx,
                                         node.inputs['Color'].links[0].from_node)

        elif node.bl_idname == "ShaderNodeRGB":
            # input rgb node
            params = ctx.color(node.color)
        else:
            raise NotImplementedError(
                f"Node type {node.bl_idname} is not supported. Only texture & RGB nodes are supported for color inputs")
    else:
        params = ctx.color(socket.default_value)

    return params


def convert_diffuse_materials_cycles(ctx, current_node):
    params = {}
    if current_node.inputs['Roughness'].is_linked or current_node.inputs['Roughness'].default_value != 0.0:
        ctx.report(
            {'WARNING'}, f"Rough diffuse BSDF '{current_node.name}' is currently not supported in Darts. Ignoring alpha parameter.")

    params.update({
        'type': 'lambertian'
    })

    color = convert_color_texture_node(ctx, current_node.inputs['Color'])

    if color is not None:
        params.update({
            'albedo': color,
        })

    return params


def roughness_to_blinn_exponent(alpha):
    return max(2.0 / (alpha * alpha) - 1.0, 0.0)


def convert_glossy_materials_cycles(ctx, current_node):
    params = {}

    if ctx.glossy_mode == 'rough conductor':
        roughness = convert_float_texture_node(ctx,
                                               current_node.inputs['Roughness'])
        if current_node.distribution != 'SHARP':
            params.update({
                'type': ctx.glossy_mode,
                'roughness': roughness,
                'distribution': RoughnessMode[current_node.distribution],
            })
        else:
            params.update({
                'type': 'conductor'
            })

        params.update({
            'color': convert_color_texture_node(ctx, current_node.inputs['Color']),
        })
    elif ctx.glossy_mode == 'blinn-phong' or ctx.glossy_mode == 'phong':
        if current_node.inputs['Roughness'].is_linked:
            raise NotImplementedError(
                f"Phong and Blinn-Phong roughness parameter doesn't support textures in Darts")
        else:
            roughness = roughness_to_blinn_exponent(
                pow(current_node.inputs['Roughness'].default_value, 2))
            if ctx.glossy_mode == 'phong':
                roughness = roughness / 4

        if current_node.distribution != 'SHARP':
            params.update({
                'type': ctx.glossy_mode,
                'exponent': roughness,
                'distribution': RoughnessMode[current_node.distribution],
            })
        else:
            params.update({
                'type': 'metal',
                'roughness': 0
            })

        params.update({
            'albedo': convert_color_texture_node(ctx, current_node.inputs['Color']),
        })
    elif ctx.glossy_mode == 'metal':
        params.update({
            'type': ctx.glossy_mode,
            'roughness': convert_float_texture_node(ctx,
                                                    current_node.inputs['Roughness']) if current_node.distribution != 'SHARP' else 0,
            'albedo': convert_color_texture_node(ctx, current_node.inputs['Color']),
        })

    return params


def convert_glass_materials_cycles(ctx, current_node):
    params = {}

    if current_node.inputs['IOR'].is_linked:
        ctx.report(
            {'WARNING'}, "Textured IOR values are not supported in Darts. Using the default value instead.")

    ior = current_node.inputs['IOR'].default_value

    roughness = convert_float_texture_node(ctx,
                                           current_node.inputs['Roughness'])

    if roughness and current_node.distribution != 'SHARP':
        params.update({
            'type': 'rough dielectric',
            'roughness': roughness,
            'distribution': RoughnessMode[current_node.distribution],
        })
    else:
        params['type'] = 'thin dielectric' if ior == 1.0 else 'dielectric'

    params['ior'] = ior

    specular_transmittance = convert_color_texture_node(ctx,
                                                        current_node.inputs['Color'])

    if specular_transmittance is not None:
        params.update({
            'specular_transmittance': specular_transmittance,
        })

    return params


def convert_emitter_materials_cycles(ctx, current_node):
    if current_node.inputs["Strength"].is_linked:
        raise NotImplementedError(
            "Only default emitter strength value is supported")
    else:
        radiance = current_node.inputs["Strength"].default_value

    if current_node.inputs['Color'].is_linked:
        raise NotImplementedError(
            "Only default emitter color is supported")  # TODO: rgb input
    else:
        radiance = [
            x * radiance for x in current_node.inputs["Color"].default_value[:]]

    if np.sum(radiance) == 0:
        ctx.report(
            {'WARN'}, "Emitter has zero emission, this may cause Darts to fail! Ignoring it.")
        return {'type': 'diffuse', 'albedo': ctx.color(0)}

    return {
        'type': 'diffuse_light',
        'emit': ctx.color(radiance),
    }


def convert_mix_materials_cycles(ctx, current_node):
    if not current_node.inputs[1].is_linked or not current_node.inputs[2].is_linked:
        raise NotImplementedError("Mix shader is not linked to two materials")

    mat_I = current_node.inputs[1].links[0].from_node
    mat_II = current_node.inputs[2].links[0].from_node

    return {
        'type': 'blend',
        'amount': convert_float_texture_node(ctx, current_node.inputs['Fac']),
        'a': cycles_material_to_dict(ctx, mat_I),
        'b': cycles_material_to_dict(ctx, mat_II)
    }


def cycles_material_to_dict(ctx, node, name=None):
    ''' Converting one material from Blender to Darts format'''

    cycles_converters = {
        "ShaderNodeBsdfDiffuse": convert_diffuse_materials_cycles,
        'ShaderNodeBsdfGlossy': convert_glossy_materials_cycles,
        'ShaderNodeBsdfGlass': convert_glass_materials_cycles,
        'ShaderNodeMixShader': convert_mix_materials_cycles,
        'ShaderNodeEmission': convert_emitter_materials_cycles,
    }

    # ctx.report({'INFO'}, f"Type = {node.type} and bl_idname = {node.bl_idname}")

    params = {}
    if name is not None:
        params["name"] = material_name(name)

    if node.bl_idname in cycles_converters:
        params.update(cycles_converters[node.bl_idname](ctx, node))
    else:
        raise NotImplementedError(
            f"Node type: {node.bl_idname} is not supported in Darts")

    if 'Normal' in node.inputs and node.inputs['Normal'].is_linked:
        normal_socket = node.inputs['Normal']

        # wrap the material in a normal map adaptor
        normal_map = {
            'type': 'normal map'
        }
        if name is not None:
            normal_map['name'] = params.pop('name')
        normal_map['normals'] = convert_color_texture_node(ctx, normal_socket)

        if normal_socket.is_linked:
            normal_node = normal_socket.links[0].from_node
            if normal_node.inputs["Strength"].is_linked:
                raise NotImplementedError(
                    "Only default normal map strength value is supported")
            else:
                normal_map['strength'] = normal_node.inputs["Strength"].default_value

        normal_map['nested'] = params

        return normal_map
    else:
        return params


def material_name(b_name):
    return f"{b_name.replace(' ', '_')}"


def get_dummy_material(ctx, name):
    params = {
        'type': 'lambertian',
        'albedo': ctx.color([1.0, 0.0, 0.3]),
    }
    if name is not None:
        params['name'] = material_name(name)
    return params


def convert_material(ctx, b_mat):
    ''' Converting one material from Blender / Cycles to Darts'''

    name = b_mat.name_full

    if b_mat.name_full in ctx.exported_mats.keys():
        return None

    if b_mat.use_nodes:
        try:
            output_node_id = 'Material Output'
            if output_node_id in b_mat.node_tree.nodes:
                output_node = b_mat.node_tree.nodes[output_node_id]
                surface_node = output_node.inputs["Surface"].links[0].from_node
                mat_params = cycles_material_to_dict(ctx,
                                                     surface_node, b_mat.name_full)
            else:
                ctx.report(
                    {'WARNING'}, f"Export of material '{b_mat.name}' failed: Cannot find material output node. Exporting a dummy material instead.")
                mat_params = get_dummy_material(ctx, b_mat.name_full)
        except NotImplementedError as e:
            ctx.report(
                {'WARNING'}, f"Export of material '{b_mat.name}' failed: {e.args[0]}. Exporting a dummy material instead.")
            mat_params = get_dummy_material(ctx, b_mat.name_full)
    else:
        mat_params = get_dummy_material(ctx, b_mat.name_full)
        mat_params.update({'albedo': ctx.color(b_mat.diffuse_color)})

    ctx.exported_mats[mat_params["name"]] = mat_params
    return mat_params


def export_materials(ctx, meshes):
    """Write out the materials to Darts format"""

    ctx.report({'INFO'}, f"Writing default lambertian material")
    mat_json = [
        {
            "type": "lambertian",
            "name": "default",
            "albedo": 0.2
        }
    ]

    if ctx.material_mode == "LAMBERTIAN":
        for mat in bpy.data.materials:
            # skip any materials that aren't being used
            if mat.name_full == "Dots Stroke" or mat.users == 0:
                continue

            ctx.report({'INFO'}, f"Writing material: {mat.name_full}")
            name = mat.name_full
            params = get_dummy_material(
                ctx, mat.name_full)
            mat_json.append(params)

    elif ctx.material_mode == "CONVERT":
        for mesh in meshes:
            for mat in mesh.data.materials:
                # skip any materials that aren't being used
                if mat.name_full == "Dots Stroke" or mat.users == 0:
                    continue

                ctx.report(
                    {'INFO'}, f"Writing material: {mat.name_full}")
                mat_params = convert_material(
                    ctx, mat)
                if mat_params is None:
                    continue
                # ctx.report({'INFO'}, json.dumps(mat_params, indent=4))
                mat_json.append(mat_params)

    return mat_json


def convert_background(ctx, world):
    """
    convert environment lighting. Constant emitter and envmaps are supported

    Params
    ------
    surface_node: the final node of the shader
    ignore_background: whether we want to export blender's default background or not
    """

    try:
        if world is None:
            ctx.report({'INFO'}, 'No Blender world to export.')
            return 0

        if world.use_nodes and world.node_tree is not None:
            output_node_id = 'World Output'
            if output_node_id not in world.node_tree.nodes:
                ctx.report(
                    {'WARNING'}, 'Failed to export world: Cannot find world output node.')
                return 0
            output_node = world.node_tree.nodes[output_node_id]
            if not output_node.inputs["Surface"].is_linked:
                return 0
            surface_node = output_node.inputs["Surface"].links[0].from_node
            if surface_node.inputs['Strength'].is_linked:
                raise NotImplementedError(
                    "Only default emitter strength value is supported")  # TODO: value input
            strength = surface_node.inputs['Strength'].default_value

            if strength == 0:  # Don't add an emitter if it emits nothing
                ctx.report({'INFO'}, 'Ignoring envmap with zero strength.')
                return 0

            if surface_node.bl_idname in ['ShaderNodeBackground', 'ShaderNodeEmission']:
                socket = surface_node.inputs["Color"]
                if socket.is_linked:
                    color_node = socket.links[0].from_node
                    if color_node.bl_idname == 'ShaderNodeTexEnvironment':
                        params = {
                            'type': 'envmap',
                            'filename': ctx.export_texture(color_node.image),
                            'scale': strength
                        }

                        if color_node.inputs["Vector"].is_linked:
                            vector_node = color_node.inputs["Vector"].links[0].from_node
                            if vector_node.bl_idname != 'ShaderNodeMapping':
                                raise NotImplementedError(
                                    f"Node: {vector_node.bl_idname} is not supported. Only a mapping node is supported")
                            if not vector_node.inputs["Vector"].is_linked:
                                raise NotImplementedError(
                                    f"The node {vector_node.bl_idname} should be linked with a Texture coordinate node")
                            coord_node = vector_node.inputs["Vector"].links[0].from_node
                            coord_socket = vector_node.inputs["Vector"].links[0].from_socket
                            if coord_node.bl_idname != 'ShaderNodeTexCoord':
                                raise NotImplementedError(
                                    f"Unsupported node type: {coord_node.bl_idname}")
                            if coord_socket.name != 'Generated':
                                raise NotImplementedError(
                                    "Link should come from 'Generated'")
                            if vector_node.inputs["Rotation"].is_linked:
                                raise NotImplementedError(
                                    "Rotation inputs shouldn't be linked")
                            params['transform'] = ctx.transform_matrix(
                                vector_node.inputs["Rotation"].default_value.to_matrix())

                        return params
                    elif color_node.bl_idname == 'ShaderNodeRGB':
                        color = color_node.color
                    else:
                        raise NotImplementedError(
                            f"Node type {color_node.bl_idname} is not supported. Consider using an environment texture or RGB node instead")
                else:
                    color = socket.default_value

                # Not an envmap
                radiance = [x * strength for x in color[:3]]
                if np.sum(radiance) == 0:
                    ctx.report(
                        {'INFO'}, "Ignoring background emitter with zero emission.")
                    return 0
                return ctx.color(radiance)
            else:
                raise NotImplementedError(
                    f"Only Background and Emission nodes are supported as final nodes for background export, got '{surface_node.name}'")
        else:
            # Single color field for emission, no nodes
            return ctx.color(world.color)

    except NotImplementedError as err:
        ctx.report(
            {'WARNING'}, f"Error while converting background: {err.args[0]}. Using default.")
        return 5