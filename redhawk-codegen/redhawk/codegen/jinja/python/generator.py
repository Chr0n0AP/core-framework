#
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of REDHAWK core.
#
# REDHAWK core is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# REDHAWK core is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see http://www.gnu.org/licenses/.
#

from redhawk.codegen.jinja.generator import CodeGenerator
from redhawk.codegen.jinja.python import PythonTemplate

class PythonCodeGenerator(CodeGenerator):
    def sourceFiles(self, component):
        for template in self.templates(component):
            if template.filename.endswith('.py'):
                yield template.filename

    def templatesChildren(self, component):
        templates = []
        if not component.get('children'):
            return templates

        for child_key in component['children']:
            templates.append({child_key: PythonTemplate('resource.py', child_key+'/'+component['children'][child_key]['userclass']['file'], userfile=True)})
            templates.append({child_key: PythonTemplate('base/__init__.py', child_key+'/__init__.py')})
            templates.append({child_key: PythonTemplate('resource_base.py', child_key+'/'+component['children'][child_key]['baseclass']['file'])})
        return templates
