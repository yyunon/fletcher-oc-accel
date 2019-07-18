#
# Copyright 2017 International Business Machines
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Make sure Fletcher environment variable is set.
ifndef FLETCHER_DIR
$(error FLETCHER_DIR is not set)
endif

all: hw sw

.PHONY: hw sw

hw:
	echo "INFO: Creating Fletcher hardware symlinks."
	rm -f ./hw/fletcher
	rm -f ./hw/sum
	ln -fs $(FLETCHER_DIR)/hardware ./hw/fletcher
	ln -fs $(FLETCHER_DIR)/examples/sum/hardware ./hw/sum

sw:
	echo "INFO: Please compile application manually."

clean:
	echo "INFO: Removing Fletcher hardware symlinks."
	rm ./hw/fletcher
	rm ./hw/sum
