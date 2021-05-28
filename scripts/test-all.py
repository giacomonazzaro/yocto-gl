#! /usr/bin/env python3 -B

import sys
import glob
import os
import subprocess
import json

mesh_dir = 'meshes'

def trace(dirname, output, algorithm, subdivisions):
    def handle_error(err, result, mesh_name, stats_name):
        result['num_errors'] += 1
        msg = 'error: ' + err
        print(msg + ' ' * max(0, 78-len(msg)))
        if err not in result:
            result[err] = []
        result[err] += [mesh_name]
        stats = {'error': err}
        with open(stats_name, 'wt') as f:
            json.dump(stats, f, indent=2)

    mesh_names = glob.glob(f'{dirname}/{mesh_dir}/*')
    mesh_num = len(mesh_names)
    
    try:
        os.mkdir(f'{output}/stats')
    except:
        pass

    result = {}
    result['num_tests'] = 0
    result['num_errors'] = 0
    result['ok'] = []

    timings_file = f'{output}/timings.csv'
    append = ''
    for mesh_id, mesh_name in enumerate(mesh_names):
        result['num_tests'] += 1
        name = os.path.basename(mesh_name).split('.')[0]
        stats_name = f'{output}/stats/{name}.json'
        scene_name = f'{output}/scenes/{name}/scene.json'
        # curve_name = name.replace('meshes/', 'curves/') + '.json'
        # scene_name = name.replace('meshes/', 'scenes/') + '.json'
        msg = f'[{mesh_id}/{mesh_num}] {mesh_name}'
        print(msg + ' ' * max(0, 78-len(msg)))

        cmd = f'./bin/splinetest {mesh_name} --algorithm {algorithm} --output {output} --trials 1 {append} --subdivisions {subdivisions} --scene {scene_name} --timings {timings_file}'
        print(cmd)
        if append == '': append = '--append-timings'
        
        try:
            retcode = subprocess.run(cmd, timeout=60, shell=True).returncode
            if retcode < 0:
                handle_error('app_terminated', result, mesh_name, stats_name)
            elif retcode > 0:
                handle_error('app_error', result, mesh_name, stats_name)
            else:
                result['ok'] += [mesh_name]
        except OSError:
            handle_error('os_error', result, mesh_name, stats_name)
        except subprocess.TimeoutExpired:
            handle_error('app_timeout', result, mesh_name, stats_name)
            
    
    with open(f'{output}/trace-result.json', 'wt') as f:
        json.dump(result, f, indent=2)

    # timings = pd.read_csv(f'{output}/timings.csv')
    # plt.scatter(timings['length'], timings['seconds'])
    # plt.savefig(f'{output}/timings.png')


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

        cmd = f'./bin/yscenetrace {scene_name}/scene.json -o {output}/{name}.png -s 1'
        # print(cmd)
        retcode = subprocess.run(cmd, timeout=600, shell=True).returncode


# algorithms = [('dc-uniform',4), ('dc-adaptive',4), ('lr-uniform',4), ('lr-adaptive',4)]
algorithms = [('dc-uniform',4)]

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
    task = ''
    
    if len(sys.argv) >= 3:
        task = sys.argv[2]

    if(task == 'render'):
        render_all(dir)
    else:
        trace_all(dir)



