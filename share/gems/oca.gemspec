
require 'pp'

DEPS=[
    'rake',
    'nokogiri',
]

Gem::Specification.new do |s|

    s.name='OpenNebula-OCA'
    s.version='1.3.80'

    s.authors=['OpenNebula Team']
    s.files=Dir['bin/*']
    s.files+=Dir['lib/*']
    s.files+=Dir['lib/OpenNebula/*.rb']

    pp s.files

    s.summary='description of the package'

    DEPS.each do |d|
        s.add_dependency d
    end
end


