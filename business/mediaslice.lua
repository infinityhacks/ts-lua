

function media_transform(data, eos)
    done = 0
    slen = string.len(data)

    if ts.ctx['fit'] > 0 then
        if ts.ctx['fit'] + slen >= ts.ctx['len'] then
            need = ts.ctx['len'] - ts.ctx['fit']
            done = 1
        else
            need = slen
        end

        ts.ctx['fit'] = ts.ctx['fit'] + need
        return data:sub(1, need), done

    else if ts.ctx['passed'] + slen >= ts.ctx['offset'] then
        need = ts.ctx['passed'] + slen - ts.ctx['offset']
        if need >= ts.ctx['len'] then
            need = ts.ctx['len']
            done = 1
        end

        spos = ts.ctx['passed'] + slen - ts.ctx['offset']
        epos = spos + need
        return data:sub(spos, epos), done
    else
        ts.ctx['passed'] = ts.ctx['passed'] + slen
        return nil, 0
    end

end


function read_response()
    http_status = ts.server_response.header.get_status()
    if http_stats ~= 200 then
        return
    end

    ts.http.resp_cache_transformed(0)
    ts.http.resp_cache_untransformed(1)

    if ts.ctx['transform_add'] == 1 then
        return
    end

    ts.hook(TS_LUA_RESPONSE_TRANSFORM, media_transform)
    ts.ctx['transform_add'] = 1
    ts.ctx['fit'] = 0
    ts.ctx['passed'] = 0
end


function cache_lookup()
    status = ts.http.get_cache_lookup_status()
    ts.debug('cache lookup status: %d', status)
    if status ~= TS_LUA_CACHE_LOOKUP_HIT_FRESH and status ~= TS_LUA_CACHE_LOOKUP_HIT_STALE then
        return
    end

    http_status = ts.cached_response.header.get_status()
    ts.debug('cached_response http status: %d', http_status)
    if http_status ~= 200 then
        return
    end

    ts.http.resp_cache_transformed(0)
    ts.hook(TS_LUA_RESPONSE_TRANSFORM, media_transform)

    ts.ctx['transform_add'] = 1
    ts.ctx['fit'] = 0
    ts.ctx['passed'] = 0
end


function do_remap()

    url = ts.client_request.get_url()

    param_s = string.find(url, '?')
    start_s, start_e, start_val = string.find(url, 'start=(%d+)', param_s)

    if start_s ~= nil then
        end_s, end_e, end_val = string.find(url, 'end=(%d+)', start_e)
        offset_s, offset_e, offset_val = string.find(url, 'offset=(%d+)', end_e)
        len_s, len_e, len_val = string.find(url, 'len=(%d+)', offset_e)
        in_query = true
    else
        start_s, start_e, start_val = string.find(url, "/start_(%d+)")
        end_s, end_e, end_val = string.find(url, "/end_(%d+)", start_e)
        offset_s, offset_e, offset_val = string.find(url, "/offset_(%d+)", end_e)
        len_s, len_e, len_val = string.find(url, "/len_(%d+)", offset_e)
    end

    if start_val == nil or end_val == nil or offset_val == nil or len_val == nil then
        ts.http.set_resp(400, "params invalid\n")
        return 0
    end

    if start_val < 0 or end_val < start_val or offset_val < 0 or len_val <= 0 then
        ts.http.set_resp(400, "params invalid\n")
        return 0
    end

    if in_query then
        slash = #url - url:reverse():find("/") + 1
        cache_url = string.format('%sstart_%d/end_%d/%s', string.sub(url, 1, slash), start_val, end_val, string.sub(url, slash+1, param_s-1))
    else
        cache_url = string.format('%s%s', string.sub(url, 1, end_e), string.sub(url, len_e))
    end

    ts.http.set_cache_url(cache_url)
    ts.client_request.header['Range'] = nil

    ts.ctx['start'] = start_val
    ts.ctx['end'] = end_val
    ts.ctx['offset'] = offset_val
    ts.ctx['len'] = len_val

    ts.hook(TS_LUA_HOOK_CACHE_LOOKUP_COMPLETE, cache_lookup)
    ts.hook(TS_LUA_HOOK_READ_RESPONSE_HDR, read_response)

    return 0
end

