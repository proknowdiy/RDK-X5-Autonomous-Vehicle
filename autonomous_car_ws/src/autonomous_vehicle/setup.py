import os
from setuptools import find_packages, setup

package_name = 'autonomous_vehicle'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        (
            'share/ament_index/resource_index/packages',
            ['resource/autonomous_vehicle'],
        ),
        (
            'share/autonomous_vehicle',
            ['package.xml'],
        ),
        (
            os.path.join('share', 'autonomous_vehicle', 'launch'),
            ['launch/autonomous_vehicle.launch.py'],
        ),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='sunrise',
    maintainer_email='sunrise@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
 #   tests_require=['pytest'],
    entry_points={
        'console_scripts': [
        ],
    },
)
