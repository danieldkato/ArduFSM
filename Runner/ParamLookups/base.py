"""Function to get specific parameters using either Hardcoded or Database

"""

import Hardcoded
try:
    import Database
    Getter = Database
except ImportError:
    print "warning: cannot import Database"
    Getter = Hardcoded


def get_specific_parameters_from_mouse_name(mouse_name):
    """Extract default board and box and use that to start session"""
    # Get mouse parameters
    mouse_parameters = Getter.get_mouse_parameters(mouse_name)
    
    # Use that to get board and box
    board = mouse_parameters['build']['default_board']
    box = mouse_parameters['build']['default_box']

    # Get board and box parameters
    board_parameters = Getter.get_board_parameters(board)
    box_parameters = Getter.get_box_parameters(box)
    
    # Boilerplate
    # Split into C, Python, and build parameters
    specific_parameters = {}
    for param_type in ['C', 'Python', 'build']:
        if param_type not in specific_parameters:
            specific_parameters[param_type] = {}
        
        specific_parameters[param_type].update(box_parameters[param_type])
        specific_parameters[param_type].update(board_parameters[param_type])
        specific_parameters[param_type].update(mouse_parameters[param_type])

    # Check the required ones are present
    for param_name in ['protocol_name', 'script_name', 'serial_port']:
        assert param_name in specific_parameters['build']
    
    # Copy some from 'build' to 'python'
    if 'serial_port' not in specific_parameters['Python']:
        specific_parameters['Python']['serial_port'] = specific_parameters[
            'build']['serial_port']
    if 'box' not in specific_parameters['Python']:
        specific_parameters['Python']['box'] = box
    if 'mouse' not in specific_parameters['Python']:
        specific_parameters['Python']['mouse'] = mouse_name
    if 'board' not in specific_parameters['Python']:
        specific_parameters['Python']['board'] = board
    
    return specific_parameters    

def get_specific_parameters_from_user_input(user_input):
    """Converts session parameters to specific parameters.
    
    """
    # Convert session parameters into specific parameters
    board_parameters = Getter.get_board_parameters(user_input['board'])
    box_parameters = Getter.get_box_parameters(user_input['box'])
    mouse_parameters = Getter.get_mouse_parameters(user_input['mouse'])    
    
    # Split into C, Python, and build parameters
    specific_parameters = {}
    for param_type in ['C', 'Python', 'build']:
        if param_type not in specific_parameters:
            specific_parameters[param_type] = {}
        
        specific_parameters[param_type].update(box_parameters[param_type])
        specific_parameters[param_type].update(board_parameters[param_type])
        specific_parameters[param_type].update(mouse_parameters[param_type])

    # Check the required ones are present
    for param_name in ['protocol_name', 'script_name', 'serial_port']:
        assert param_name in specific_parameters['build']
    
    # Copy some from 'build' to 'python'
    if 'serial_port' not in specific_parameters['Python']:
        specific_parameters['Python']['serial_port'] = specific_parameters[
            'build']['serial_port']
    if 'box' not in specific_parameters['Python']:
        specific_parameters['Python']['box'] = user_input['box']
    if 'mouse' not in specific_parameters['Python']:
        specific_parameters['Python']['mouse'] = user_input['mouse']
    if 'board' not in specific_parameters['Python']:
        specific_parameters['Python']['board'] = user_input['board']
    
    return specific_parameters

def translate_c_parameter_name(name):
    """Translate C parameters from human-readable to C-mangled.
    
    For example: if name is 'side_HE_sensor_thresh', this function returns
        '__HWCONSTANTS_H_HALL_THRESH'
    
    If the name is not recognized, returns None.
    
    """
    if name == 'side_HE_sensor_thresh':
        return '__HWCONSTANTS_H_HALL_THRESH'
    elif name == 'use_ir_detector':
        return '__HWCONSTANTS_H_USE_IR_DETECTOR'
    elif name == 'stepper_driver':
        return '__HWCONSTANTS_H_USE_STEPPER_DRIVER'
    elif name == 'microstep':   
        return '__HWCONSTANTS_H_MICROSTEP'
    elif name == 'invert_stepper_direction':
        return '__HWCONSTANTS_H_INVERT_STEPPER_DIRECTION'
    else:
        return None