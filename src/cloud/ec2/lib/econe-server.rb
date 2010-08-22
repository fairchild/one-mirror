# -------------------------------------------------------------------------- #
# Copyright 2002-2010, OpenNebula Project Leads (OpenNebula.org)             #
#                                                                            #
# Licensed under the Apache License, Version 2.0 (the "License"); you may    #
# not use this file except in compliance with the License. You may obtain    #
# a copy of the License at                                                   #
#                                                                            #
# http://www.apache.org/licenses/LICENSE-2.0                                 #
#                                                                            #
# Unless required by applicable law or agreed to in writing, software        #
# distributed under the License is distributed on an "AS IS" BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
# See the License for the specific language governing permissions and        #
# limitations under the License.                                             #
#--------------------------------------------------------------------------- #

##############################################################################
# Environment Configuration for the Cloud Server
##############################################################################
ONE_LOCATION=ENV["ONE_LOCATION"]

if !ONE_LOCATION
    RUBY_LIB_LOCATION  = "/usr/lib/one/ruby"
    CONFIGURATION_FILE = "/etc/one/econe.conf"
    TEMPLATE_LOCATION  = "/etc/one/ec2query_templates"
else
    RUBY_LIB_LOCATION  = ONE_LOCATION+"/lib/ruby"
    CONFIGURATION_FILE = ONE_LOCATION+"/etc/econe.conf"
    TEMPLATE_LOCATION  = ONE_LOCATION+"/etc/ec2query_templates"
end

VIEWS_LOCATION = RUBY_LIB_LOCATION + "/cloud/econe/views"

$: << RUBY_LIB_LOCATION
$: << RUBY_LIB_LOCATION+"/cloud"
$: << RUBY_LIB_LOCATION+"/cloud/econe"

###############################################################################
# Libraries
###############################################################################
require 'rubygems'
require 'sinatra'

require 'EC2QueryServer'

include OpenNebula

#
# generate a parseable AWS-style error response
# error_code should correspond to http://docs.amazonwebservices.com/AmazonS3/2006-03-01/API/index.html?ErrorCodeList.html
#
def to_aws_error(msg, error_code = "InternalError")
        "<?xml version=\"1.0\"?><Response><Errors><Error><Code>#{error_code}</Code><Message>#{msg}</Message></Error></Errors><RequestID>0</RequestID></Response>"
end

$econe_server = EC2QueryServer.new(CONFIGURATION_FILE,
    TEMPLATE_LOCATION, VIEWS_LOCATION)

##############################################################################
# Sinatra Configuration
##############################################################################
set :host, $econe_server.config[:server]
set :port, $econe_server.config[:port]

##############################################################################
# Actions
##############################################################################

before do
    if !$econe_server.authenticate?(params)
        halt 401, to_aws_error('Invalid credentials')
    end
end

post '/' do
    case params['Action']
        when 'UploadImage'
            result,rc = $econe_server.upload_image(params)
        when 'RegisterImage'
            result,rc = $econe_server.register_image(params)
        when 'DescribeImages'
            result,rc = $econe_server.describe_images(params)
        when 'RunInstances'
            result,rc = $econe_server.run_instances(params)
        when 'DescribeInstances'
            result,rc = $econe_server.describe_instances(params)
        when 'TerminateInstances'
            result,rc = $econe_server.terminate_instances(params)
    end
    
    if OpenNebula::is_error?(result)
        halt rc, to_aws_error(result.message)
    end

    result
end
