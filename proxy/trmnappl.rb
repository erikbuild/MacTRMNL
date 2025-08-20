#!/usr/bin/env ruby

require 'socket'
require 'net/http'
require 'json'
require 'uri'

class TRMNLProxy
  DEFAULT_PORT = 1337
  TRMNL_API_BASE = 'https://usetrmnl.com'
  
  def initialize(port = DEFAULT_PORT)
    @port = port
    @access_token = ENV['ACCESS_TOKEN']
    
    if @access_token.nil? || @access_token.empty?
      puts "Error: ACCESS_TOKEN environment variable is required"
      exit 1
    end
    
    puts "Starting TRMNL proxy server on port #{@port}"
  end
  
  def start
    server = TCPServer.new(@port)
    
    loop do
      begin
        client = server.accept
        puts "Client connected from #{client.peeraddr[3]}"
        
        handle_client(client)
        
      rescue => e
        puts "Error handling client: #{e.message}"
        puts e.backtrace.join("\n")
      ensure
        client&.close
        puts "Client disconnected"
      end
    end
  end
  
  private
  
  def handle_client(client)
    puts "Fetching display data from TRMNL API..."
    
    display_data = fetch_display_data
    return unless display_data
    
    image_url = display_data['image_url']
    if image_url.nil? || image_url.empty?
      puts "No image URL in response"
      return
    end
    
    puts "Fetching image from: #{image_url}"
    image_data = fetch_image(image_url)
    return unless image_data
    
    puts "Streaming BMP data to client (#{image_data.length} bytes)..."
    client.write(image_data)
    puts "Image sent successfully"
  end
  
  def fetch_display_data
    uri = URI("#{TRMNL_API_BASE}/api/display")
    
    http = Net::HTTP.new(uri.host, uri.port)
    http.use_ssl = true
    
    request = Net::HTTP::Get.new(uri)
    request['Access-Token'] = @access_token
    
    response = http.request(request)
    
    if response.code == '200'
      JSON.parse(response.body)
    else
      puts "API request failed: #{response.code} #{response.message}"
      puts response.body
      nil
    end
  rescue => e
    puts "Error fetching display data: #{e.message}"
    nil
  end
  
  def fetch_image(image_url)
    uri = URI(image_url)
    
    http = Net::HTTP.new(uri.host, uri.port)
    http.use_ssl = (uri.scheme == 'https')
    
    request = Net::HTTP::Get.new(uri)
    
    # Follow redirects
    5.times do
      response = http.request(request)
      
      case response.code
      when '200'
        return response.body
      when '301', '302', '303', '307', '308'
        redirect_url = response['location']
        puts "Following redirect to: #{redirect_url}"
        uri = URI(redirect_url)
        http = Net::HTTP.new(uri.host, uri.port)
        http.use_ssl = (uri.scheme == 'https')
        request = Net::HTTP::Get.new(uri)
      else
        puts "Image fetch failed: #{response.code} #{response.message}"
        return nil
      end
    end
    
    puts "Too many redirects"
    nil
  rescue => e
    puts "Error fetching image: #{e.message}"
    nil
  end
end