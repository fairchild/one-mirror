
require 'pp'

DEPS=[
    'amazon-ec2'
]

Gem::Specification.new do |s|

    s.name='OpenNebula-ECOne'
    s.version='1.4'

    s.authors=['OpenNebula Team', 'claude.noshpitz@attinteractive.com']
    s.files=Dir['bin/*']
    s.files+=Dir['lib/*']
    s.files+=Dir['lib/econe/*.rb']

    pp s.files

    s.summary='EC2-style CLI for Open Nebula'

    DEPS.each do |d|
        s.add_dependency d
    end
end


