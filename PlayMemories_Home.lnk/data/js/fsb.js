console.clear();

Array.prototype.clear = function ()
{
	this.splice(0, this.length);
}

Vue.component('content-item',
	{
		props: [ 'item','active' ],
		template: document.querySelector('#templates #item')
	});

var app1 = new Vue({
	el: '#app1',
	data: {
		directory: "",
		uploading: "",
		content: [],
		selected:null
	},
	methods: {
		itemClicked: function (item)
		{
			var _this = this;
			if (item.icon.indexOf('folder') >= 0)
			{
				if (item.name != '..')
				{
					if (item.directory != '') item.directory += '/';
					this.directory += item.name;
				}
				else
				{
					var paths = this.directory.split('/');
					paths.pop();
					this.directory = paths.join('/');
				}
				this.content.clear();
	
				var kdir = getDirectory('/' + this.directory);
				
				if (this.directory != '')
				{
					this.content.push({
						name: '..',
						icon: 'folder back',
					});
				}
	
				kdir.forEach(function (item)
				{
					_this.content.push(item);
				});
			}
			else
			{
				this.selected = item;
			}

		}
	}
});

function loadDirectory()
{
	return new Promise(function (res, rej)
	{
		var req = new XMLHttpRequest();
		req.open("GET", "/listdir?path=/");
		req.onload = function ()
		{
			directory = JSON.parse(req.response);
			res();
		}
		req.send();
	});
}

function getDirectory(path)
{
	if (!path.startsWith('/')) path = '/' + path;
	if (!path.endsWith('/')) path += '/'
	var dir = directory.filter(function (a)
	{
		return a.name.startsWith(path);
	});

	var files = [];
	var directories = []

	dir.forEach(function (a)
	{
		var isFile = false;
		var left = a.name.substring(path.length, a.name.length);
		if (left.indexOf('/') == -1) isFile = true;

		if (isFile)
		{
			files.push({ name: left, icon: 'file', fullname:a.name });
		}
		else
		{

			var left = left.split('/')[ 0 ];
			if (directories.indexOf(left) == -1)
				directories.push(left);
		}
	});

	directories.forEach(function (a)
	{
		files.splice(0, 0, {
			name: a,
			icon: 'folder',
			fullname:a
		});
	});
	files.sort(function(a,b)
	{
		if(a.icon=='folder' && b.icon=='file')return -1;
		if(a.icon=='file' && b.icon=='folder')return 1;

		if(a.name<b.name)return -1
		if(a.name>b.name)return 1;
		return 0;
	});
	return files;
}

var directory;

loadDirectory().then(function ()
{
	var ret = getDirectory("/");
	ret.forEach(function (item)
	{
		app1.content.push(item);
	});
});

function uploadHandler()
{
	var inp = document.createElement('input');
	inp.type = 'file';
	document.body.appendChild(inp);
	inp.click();
	inp.multiple = false;
	document.body.removeChild(inp);
	inp.onchange = function ()
	{
		if (inp.files.length === 0)
		{
			return
		}
		var fr1 = new FileReader();
		var file = inp.files[ 0 ];

		fr1.onload = function ()
		{
			var req = new XMLHttpRequest();
			req.onload = function ()
			{
				alert('done');
			}
			var formData = new FormData();
			formData.append('path', mergePath(app1.directory, file.name));
			var data = fr1.result;
			data = data.split(',')[ 1 ];
			formData.append('file', data);

			req.open("POST", "/upload");
			req.send(formData)
		}
		fr1.readAsDataURL(file);
		//document.getElementById('pushFile').click();
	}

}

function deleteHandler()
{
	var req = new XMLHttpRequest();
	req.open("GET","/delete?path=" + app1.selected.fullname);
	req.send();
}

function downloadHandler()
{
	window.open(app1.selected.fullname + "?download=true","_blank");
	
}

/**
 * @param {string} a
 * @param {string} b
 * @returns {string}
 */
function mergePath(a, b)
{
	if (a.endsWith('/'))
		a = a.substring(0, a.length - 1);
	if (b.startsWith('/'))
		b = b.substring(1, b.length);
	return a + '/' + b;
}