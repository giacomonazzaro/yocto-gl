#! /usr/bin/env python3 -B

import sys
import glob
import os
import subprocess
import json

mesh_dir = 'meshes'

def trace(dir, mesh_name, algorithm, trial):
    try:
        os.mkdir(f'{dir}/{algorithm}')
    except:
        pass
    try:
        os.mkdir(f'{dir}/{algorithm}/images')
    except:
        pass
    try:
        os.mkdir(f'{dir}/{algorithm}/scenes')
    except:
        pass

    name = os.path.basename(mesh_name).split('.')[0]
    scene_name = f'{dir}/{algorithm}/scenes/{name}/scene.json'

    try:
        os.mkdir(f'{dir}/{algorithm}/scenes/{name}')
    except:
        pass    
    # curve_name = name.replace('meshes/', 'curves/') + '.json'
    # scene_name = name.replace('meshes/', 'scenes/') + '.json'
    # msg = f'[{mesh_id}/{mesh_num}] {mesh_name}'
    # print(msg + ' ' * max(0, 78-len(msg)))

    # cmd = f'./bin/splinetest {dir}/meshes/{mesh_name} -a {algorithm} -t {trial} --subdivisions 4 --scene {scene_name}'
    cmd = f'../yocto-master/bin/splinetest {dir}/meshes/{mesh_name} --algorithm {algorithm} --selected-trial {trial} --subdivisions 4 --scene {scene_name}'
    
    print(cmd)
    retcode = subprocess.run(cmd, timeout=600, shell=True).returncode
    
    cmd = f'./bin/yscenetrace {scene_name} -o {dir}/{algorithm}/images/{name}.png -s 8'
    print(cmd)
    retcode = subprocess.run(cmd, timeout=600, shell=True).returncode
    


def render(dirname, output):
    
    scene_names = glob.glob(f'{dirname}/scenes/*')
    print(dirname)
    try:
        os.mkdir(f'{output}')
    except:
        pass

    # result = {}
    # result['num_tests'] = 0
    # result['num_errors'] = 0
    # result['ok'] = []
    mesh_num = len(scene_names)


    for mesh_id, scene_name in enumerate(scene_names):
        # result['num_tests'] += 1
        name = os.path.basename(scene_name).split('.')[0]
        # stats_name = f'{output}/stats/{name}.json'
        # scene_name = f'{output}/scenes/{name}/scene.json'
        # curve_name = name.replace('meshes/', 'curves/') + '.json'
        # scene_name = name.replace('meshes/', 'scenes/') + '.json'
        msg = f'[{mesh_id}/{mesh_num}] {scene_name}'
        print(msg + ' ' * max(0, 78-len(msg)))

        cmd = f'./bin/yscenetrace {scene_name}/scene.json -o {output}/{name}.png -s 256'
        print(cmd)
        retcode = subprocess.run(cmd, timeout=600, shell=True).returncode


# algorithms = [('dc-uniform',4), ('dc-adaptive',4), ('lr-uniform',4), ('lr-adaptive',4)]
algorithms = [('lr-uniform',4)]


def trace_all(dir):
    for algorithm, subdivisions in algorithms:
        output = f'{dir}/{algorithm}-{subdivisions}'
        
        try:
            os.mkdir(output)
        except:
            pass
        trace(dir, output, algorithm, subdivisions)
    

def render_all(dir):
    for algorithm, subdivisions in algorithms:
        output = f'{dir}/{algorithm}-{subdivisions}/images'
        
        try:
            os.mkdir(output)
        except:
            pass
        render(f'{dir}/{algorithm}-{subdivisions}', output)


if __name__ == '__main__':
    dir = sys.argv[1]
    mesh = sys.argv[2]
    trial = sys.argv[3]
    for a in algorithms:
        trace(dir, mesh, a[0], trial)
    
    # render(dir, mesh)



