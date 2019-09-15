var divOutput = document.getElementById('output');
var divControl = document.getElementById('savedControl');
var can = document.querySelector('canvas');

var btnrec = document.getElementById('rec');
var btnfet = document.getElementById('fet');

var btnSave = document.getElementById('btnSave');
var btnPlayback = document.getElementById('btnPlayback');


/**@type {HTMLInputElement} */
var inpScale = document.getElementById('scale');
var inpName = document.getElementById('inpName');

btnrec.onclick = function ()
{
    var req = new XMLHttpRequest();
    req.open("GET", "/record.service");
    req.send();
    req.onload = function ()
    {
        divOutput.innerText = req.response
    }
}

/**@type {number[]} */
var timecode;

btnfet.onclick = function ()
{
    var req = new XMLHttpRequest();
    req.open("GET", "/fetchRecord.service");
    req.send();
    req.onload = function ()
    {
        var jobj = JSON.parse(req.response);
        timecode = jobj.timecode;
        drawGraph();
    }
}

function drawGraph()
{
    var minSpan = timecode[ 1 ] - timecode[ 0 ];

    for (var i = 1; i < timecode.length - 1; i++)
    {
        var span = timecode[ i + 1 ] - timecode[ i ];
        if (span < minSpan)
            minSpan = span;
    }

    var aw = can.clientWidth;
    var ah = can.clientHeight;
    var offx = timecode[ 0 ];
    var maxx = timecode[ timecode.length - 1 ];
    var offMaxx = maxx - offx;

    var scalex = minSpan / 20;
    inpScale.placeholder = scalex.toString();
    
    try
    {
        var sugx = inpScale.value;
        sugx = parseFloat(sugx);
        if(!isNaN(sugx) && isFinite(sugx))
            scalex = sugx;
    }
    catch(ex){}
    
    offMaxx /= scalex;
    can.width = Math.ceil(offMaxx) + 5;

    var highy = 5;
    var lowy = ah - 10;

    var state = true;
    var ctx = can.getContext('2d');

    ctx.fillStyle = '#FFFFFF';
    ctx.strokeStyle = '#FF0000'

    ctx.clearRect(0, 0, aw, ah);

    ctx.fillStyle = '#000000';
    ctx.font = 'sans-serif 5px';
    ctx.moveTo(5, lowy);

    for (var i = 0; i < timecode.length - 1; i++)
    {
        var xc1 = timecode[ i ];
        var xc2 = timecode[ i + 1 ];

        xc1 -= offx;
        xc2 -= offx;

        xc1 /= scalex;
        xc2 /= scalex;

        xc1 += 5;
        xc2 += 5;

        if (state)
        {
            ctx.lineTo(xc1, highy);
            ctx.lineTo(xc2, highy);
        }
        else
        {
            ctx.lineTo(xc1, lowy);
            ctx.lineTo(xc2, lowy);
        }
        ctx.stroke();

        var tm = ctx.measureText(i.toString());

        ctx.fillText(i.toString(), xc1 - tm.width / 2, lowy + 10);

        state = !state;
    }
}

btnSave.onclick = function ()
{
    if (inpName.value.trim() != '')
    {
        divOutput.innerText = '';
        var req = new XMLHttpRequest();
        req.open('GET', '/saveRecord.service?name=' + inpName.value);
        req.send();
        req.onload = function ()
        {
            divOutput.innerText = req.response;
        }
    }
    else
    {
        alert('please insert name');
        inpName.focus();
    }
}

btnPlayback.onclick = function ()
{
    divOutput.innerText = '';
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/playback.service');
    xhr.send();
    xhr.onload = function ()
    {
        divOutput.innerHTML = xhr.response;
    }
}

function loadExistingSignal()
{
    var req = new XMLHttpRequest();
    req.open("GET", "/listRecord.service");
    req.send();
    req.onload = function ()
    {
        var obj = JSON.parse(req.response);
        if (obj.d)
        {
            var html = "";
            obj.d.forEach(function (item)
            {
                html += "<div>";
                html += item.name;
                html += "<button onclick='repeatSig(\"" + item.name + "\")'>Repeat</button><button onclick='deleteSig(\"" + item.name + "\")'>Delete</button>";
            });

            divControl.innerHTML = html;
        }
    }
}
loadExistingSignal();

function repeatSig(signame)
{
    divOutput.innerText = '';
    var req = new XMLHttpRequest();
    req.open("GET", "/loadsignal.service?sig=" + signame);
    req.send();
    req.onload = function ()
    {
        divOutput.innerText = req.response;
    }
}

function deleteSig(signame)
{
    console.log(signame);
}

(function trimFnc()
{
    var inpTrim = document.getElementById('trim');
    var btnTrim = document.getElementById('btnTrim');

    btnTrim.onclick = function ()
    {
        divOutput.innerText = '';
        util.get("trim.service?value=" + inpTrim.value).then(function(resp)
        {
            divOutput.innerText = resp;
        });
    }

})();

var util = (function utilFunc()
{
    return {
        get: get1=function (url)
        {
            var cbb;
            var ret = { then: function (cb) { cbb = cb; } };
            var req = new XMLHttpRequest();
            req.open("GET", url);
            req.send();
            req.onload = function ()
            {
                if (typeof cbb == 'function')
                    cbb(req.response);
            }
            return ret;
        },
        post: post1=function (url, data)
        {
            var cbb;
            var ret = { then: function (cb) { cbb = cb; } };
            var req = new XMLHttpRequest();
            req.open("POST", url);
            req.send(data);
            req.onload = function ()
            {
                if (typeof cbb == 'function')
                    cbb(req.response);
            }
            return ret;
        }
    }
})();