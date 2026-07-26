from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
# from ament_index_python.packages import (
#     get_package_share_directory,
#     get_package_prefix,
# )
import shutil
from pathlib import Path
from ament_index_python.packages import (
    get_package_share_directory,
    get_package_prefix,
)

import os


def generate_launch_description():

    ####################################################################
    # Shared Memory
    ####################################################################

    shared_mem = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("hobot_shm"),
                "launch",
                "hobot_shm.launch.py"
            )
        )
    )

    ####################################################################
    # MIPI Camera
    ####################################################################

    camera = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("mipi_cam"),
                "launch",
                "mipi_cam_640x480_nv12_hbmem.launch.py"
            )
        )
    )

    ####################################################################
    # JPEG Encoder
    ####################################################################

    jpeg_codec = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("hobot_codec"),
                "launch",
                "hobot_codec_encode.launch.py",
            )
        ),
        launch_arguments={
            "codec_in_mode": "shared_mem",
            "codec_out_mode": "ros",
            "codec_sub_topic": "/hbmem_img",
            "codec_pub_topic": "/image",
        }.items(),
    )

    ####################################################################
    # WebSocket
    ####################################################################

    websocket_left = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("websocket"),
                "launch",
                "websocket.launch.py"
            )
        ),
        launch_arguments={
            "websocket_image_topic": "/image",  # /lane_debug_jpeg # /image # /lane_debug_image
            "websocket_image_type": "mjpeg",
            "websocket_only_show_image": "False", # True
            "websocket_smart_topic": "/hobot_dnn_detection",  # /lane_debug
            "websocket_channel": "0",
        }.items(),
    )

    websocket_right = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("websocket"),
                "launch",
                "websocket.launch.py"
            )
        ),
        launch_arguments={
            "websocket_image_topic": "/lane_debug_jpeg",  # /lane_debug_jpeg # /image # /lane_debug_image
            "websocket_image_type": "mjpeg",
            "websocket_only_show_image": "True", # True
            "websocket_smart_topic": "/lane_debug",  # /lane_debug
            "websocket_channel": "1",
        }.items(),
    )

    ####################################################################
    # Copy DNN config directory (required by dnn_node_example)
    ####################################################################

    dnn_config_src = Path(
        get_package_prefix("dnn_node_example")
    ) / "lib" / "dnn_node_example" / "config"

    dnn_config_dst = Path.cwd() / "config"

    shutil.copytree(
        dnn_config_src,
        dnn_config_dst,
        dirs_exist_ok=True
    )

    ####################################################################
    # Lane Detection
    ####################################################################

    lane_detector_node = Node(
        package="lane_detection",
        executable="lane_detector_node",
        name="lane_detector_node",
        output="screen",
        arguments=["--ros-args", "--log-level", "warn"],
    )

    ####################################################################
    # yolo
    ####################################################################

    # dnn_config = os.path.join(
    #     get_package_prefix("dnn_node_example"),
    #     "lib",
    #     "dnn_node_example",
    #     "config",
    #     "fcosworkconfig.json",
    # )

    yolo_node = Node(
        package="dnn_node_example",
        executable="example",
        output="screen",
        parameters=[
            {"config_file": "config/yolov11workconfig.json"},   # config/fcosworkconfig.json
            {"dump_render_img": 0},
            {"feed_type": 1},
            {"is_shared_mem_sub": 1},
            {"msg_pub_topic_name": "hobot_dnn_detection"},
        ],
        arguments=["--ros-args", "--log-level", "warn"],
    )

    ####################################################################
    # Vehicle Controller
    ####################################################################

    vehicle_controller_node = Node(
        package="vehicle_controller",
        executable="vehicle_controller_node",
        name="vehicle_controller_node",
        output="screen",
        arguments=["--ros-args", "--log-level", "info"],
    )

    return LaunchDescription([
        # Infrastructure
        shared_mem,
        camera,
        jpeg_codec,

        # AI
        lane_detector_node,
        yolo_node,

        # Control
        vehicle_controller_node,

        # Visualization
        websocket_left,                    #websocket,
        websocket_right,
    ])